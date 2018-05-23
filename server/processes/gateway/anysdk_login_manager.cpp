/*
 * Author: LiZhaojia - waterlzj@gmail.com
 *
 * Last modified: 2017-06-29 01:28 +0800
 *
 * Description: 
 */


#include "anysdk_login_manager.h"

#include "gateway.h"

#include "componet/logger.h"
#include "componet/scope_guard.h"
#include "componet/json.h"

#include "net/http_packet.h"

#include "coroutine/coroutine.h"

#include <sys/socket.h>  
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> 


//dns解析代码 //很丑陋, 先将就吧
std::pair<std::string, bool> resolveHost(const std::string& hostname )   
{  
    struct addrinfo *answer, hint, *curr;  
    char ipstr[16];                                                                                                          
    bzero(&hint, sizeof(hint));  
    //hint.ai_family = AF_UNSPEC;
    hint.ai_family = AF_INET;  
    hint.ai_socktype = SOCK_STREAM;  

    int retc = getaddrinfo(hostname.c_str(), NULL, &hint, &answer);  
    if (retc != 0)  
        return {gai_strerror(retc), false};  

    std::string ret;
    for (curr = answer; curr != NULL; curr = curr->ai_next) 
    {   
        inet_ntop(AF_INET, &(((struct sockaddr_in *)(curr->ai_addr))->sin_addr), ipstr, 16);  
        ret = ipstr;
        break; //拿到一个立刻返回
    }   
    freeaddrinfo(answer);  
    return {ret, true};
}


namespace gateway{

AnySdkLoginManager AnySdkLoginManager::s_me;

AnySdkLoginManager& AnySdkLoginManager::me()
{
    return s_me;
}

//////////////////////////////////////////////////////////////////////////
AnySdkLoginManager::AnySdkLoginManager()
{
}

void AnySdkLoginManager::onNewHttpConnection(net::BufferedConnection::Ptr conn)
{
    HttpConnectionId hcid = genCliHttpConnectionId();
    LOG_TRACE("ASS, new client http connection, hcid={}, remote={}", hcid, conn->getRemoteEndpoint());
    auto& conns = Gateway::me().httpConnectionManager();
    if (!conns.addConnection(hcid, conn, HttpConnectionManager::ConnType::client))
    {
        LOG_ERROR("ASS, insert cli conn to tcpConnManager failed, hcid={}", hcid);
        return;
    }

    //create a new client
    auto client = AllClients::AnySdkClient::create();
    client->clihcid = hcid;
    client->assip = &m_assip;
    client->tokens = &m_tokens;
    client->now = &m_now;
    {
        std::lock_guard<componet::Spinlock> lock(m_allClients.newClentsLock);
        if (!m_allClients.newClients.insert({hcid, client}))
        {
            LOG_ERROR("ASS, insert client to corotCliets failed, hcid={}", client->clihcid);
            conns.eraseConnection(hcid);
            return;
        }
    }
}

void AnySdkLoginManager::dealHttpPackets(componet::TimePoint now)
{
    auto& conns = Gateway::me().httpConnectionManager();

    HttpConnectionManager::ConnectionHolder::Ptr conn;
    net::HttpPacket::Ptr packet;
    while(conns.getPacket(&conn, &packet))
    {
        const HttpConnectionId clihcid = isCliHcid(conn->hcid) ? conn->hcid : conn->hcid - 1;
        const HttpConnectionId asshcid = clihcid + 1;
        auto iter = m_allClients.corotClients.find(clihcid);
        if (iter == m_allClients.corotClients.end())
        {
            LOG_DEBUG("ASS, deal packet, client has gone, conn->hcid={}", conn->hcid);
            conns.eraseConnection(clihcid);
            conns.eraseConnection(clihcid + 1);
            continue;
        }

        auto client = iter->second;
        if (conn->type == HttpConnectionManager::ConnType::client)
        {
            if (client->status != AllClients::AnySdkClient::Status::recvCliReq)
            {
                LOG_ERROR("ASS, deal packet, recvd cli packet under status={}, conn->hcid={}", client->status, conn->hcid);
                conns.eraseConnection(clihcid);
                conns.eraseConnection(clihcid + 1);
                continue;
            }

            client->cliReq = packet;
            client->status = AllClients::AnySdkClient::Status::connToAss;
            LOG_DEBUG("ASS, deal packet, recved cli packet, status={}, conn->hcid={}", client->status, conn->hcid);
        }
        else
        {
            if (client->status != AllClients::AnySdkClient::Status::recvAssRsp)
            {
                LOG_ERROR("ASS, deal packet, recvd ass packet under status={}, conn->hcid={}", client->status, conn->hcid);
                conns.eraseConnection(clihcid);
                conns.eraseConnection(clihcid + 1);
                continue;
            }
            client->assRsp = packet;
            client->status = AllClients::AnySdkClient::Status::rspToCli;
            conns.eraseConnection(asshcid);
            LOG_DEBUG("ASS, deal packet, recved ass packet, status={}, conn->hcid={}", client->status, conn->hcid);
        }
    }
}

void AnySdkLoginManager::afterClientDisconnect(HttpConnectionId hcid)
{
    std::lock_guard<componet::Spinlock> lock(m_closedHcidsLock);
    m_closedHcids.push_back(hcid);
}

bool AnySdkLoginManager::isAssHcid(HttpConnectionId hcid)
{
    return hcid % 2 == 0;
}

bool AnySdkLoginManager::isCliHcid(HttpConnectionId hcid)
{
    return hcid % 2 != 0;
}

HttpConnectionId AnySdkLoginManager::genCliHttpConnectionId() const
{
    return m_lastHcid += 2;
}


void AnySdkLoginManager::startNameResolve()
{
    auto resolve = [this]()
    {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        while (true)
        {
            const auto& resolveRet = resolveHost("oauth.anysdk.com");
            if (!resolveRet.second)
            {
                LOG_TRACE("resolve host oauth.anysdk.com failed, will retry in 5 seconds ...");
                std::this_thread::sleep_for(std::chrono::seconds(5));
                continue;
            }

//            LOG_TRACE("resolve host oauth.anysdk.com ok, ip={}, will expire in 60 seconds ...", resolveRet.first);
            this->m_assip.lock.lock();
            this->m_assip.ipstr = resolveRet.first;
            this->m_assip.lock.unlock();
            std::this_thread::sleep_for(std::chrono::seconds(60));
        }
    };

    std::thread resolver(resolve);
    resolver.detach(); //自由运行
}


bool AnySdkLoginManager::checkAccessToken(const std::string& openid, const std::string& token) const
{
    auto iter = m_tokens.find(openid);
    if (iter == m_tokens.end())
        return false;
    return iter->second->token == token;
}

void AnySdkLoginManager::AllClients::AnySdkClient::corotExec()
{
    HttpConnectionId asshcid = clihcid + 1;
    auto& conns = Gateway::me().httpConnectionManager();
    ON_EXIT_SCOPE_DO(LOG_TRACE("ASS, CROTO EXIT successed, hcid={}", clihcid));
    ON_EXIT_SCOPE_DO(status = Status::destroy);
    while (true)
    {
        switch (status)
        {
        case Status::recvCliReq:
            {
                corot::this_corot::yield();
                break;
            }
        case Status::connToAss:
            {
                //conn to ass
                std::string ipstr;
                net::Endpoint ep;
                {
                    std::lock_guard<componet::Spinlock> lock(assip->lock);
                    ipstr = assip->ipstr;
                }
                ep.ip.fromString(ipstr);
                ep.port = 80;
                auto conn = net::TcpConnection::create(ep);
                try
                {
                    while(!conn->tryConnect())
                        corot::this_corot::yield();
                }
                catch (const net::NetException& ex)
                {
                    LOG_ERROR("ASS, conn to ass failed, hcid={}, ipstr={}", asshcid, ipstr);
                    status = Status::assAbort;
                    break;
                }

                //reg conn
                auto assconn = net::BufferedConnection::create(std::move(*conn));
                assconn->setNonBlocking();
                if (!conns.addConnection(asshcid, assconn, HttpConnectionManager::ConnType::server))
                {
                    LOG_ERROR("ASS, insert ass conn to tcpConnManager failed, hcid={}", clihcid);
                    status = Status::assAbort;
                    break;
                }
                status = Status::reqToAss;
                LOG_TRACE("ASS, conn to ass successed, hcid={}, ipstr={}", asshcid, ipstr);
            }
        case Status::reqToAss:
            {
                std::string pattern =
                "POST /api/User/LoginOauth/ HTTP/1.1\r\n"
                "Host: oauth.anysdk.com\r\n"
                "User-Agent: caonimaanysdk\r\n"
                "Accept: */*\r\n"
                "Content-Type: application/x-www-form-urlencoded\r\n"
                "Content-Length: {}\r\n"
                "\r\n"
                "{}";
                const std::string& body = cliReq->msg().body;
                std::string reqBuf = componet::format(pattern, body.size(), body);
                const auto& ret = net::HttpPacket::tryParse(net::HttpMsg::Type::request, reqBuf.data(), reqBuf.size());
                if (ret.first == nullptr)
                {   
                    LOG_ERROR("ASS, send to ass, tryParse failed, hcid={}", clihcid);
                    status = Status::assAbort;
                    break;
                }   
                auto packet = ret.first;
                if (!conns.sendPacket(asshcid, packet))
                {
                    corot::this_corot::yield();
                    break;
                }
                LOG_TRACE("ASS, send request to ass, hcid={}, packetsize={}, reqbufsize={}", clihcid, packet->size(), reqBuf.size());
                status = Status::recvAssRsp;
            }
        case Status::recvAssRsp:
            {
                corot::this_corot::yield();
                break;
            }
        case Status::rspToCli:
            {
                std::string body = assRsp->msg().body;
                const std::string& jsonraw = assRsp->msg().body;
                try
                {
                    auto j = componet::json::parse(jsonraw);
                    if (j["status"].is_string() && j["status"] == "ok")
                    {
                        auto tokenInfo = AnySdkLoginManager::TokenInfo::create();
                        tokenInfo->openid    = j["data"]["openid"];
                        tokenInfo->token     = j["data"]["access_token"];
                        time_t rcvExpiresIn  = j["data"]["expires_in"];
                        time_t expiresIn = rcvExpiresIn;

                        tokenInfo->expiry = expiresIn + componet::toUnixTime(*now);
                        LOG_TRACE("ASS, ass response parse successed, hcid={}, openid={}, token={}, rcvExpiresIn={}", 
                                  clihcid, tokenInfo->openid, tokenInfo->token, rcvExpiresIn);
                        (*tokens)[tokenInfo->openid] = tokenInfo;

                        //用data的值填充ext, 以便cli编程抓取
                        j["ext"] = j["data"];
                        body = j.dump();
                    }
                }
                catch (const std::exception& ex)
                {
                    LOG_ERROR("ASS, ass response unexpected data, json parse failed, hcid={}, rawdata={}, ex={}", clihcid, jsonraw, ex);
                    status = Status::assAbort;
                    conns.eraseConnection(asshcid);
                    break;
                }
                std::string pattern =
                "HTTP/1.1 200 OK\r\n"
                "Server: Tengine/1.5.2\r\n"
                "Date: Tue, 27 Jun 2017 18:45:56 GMT\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: {}\r\n"
                "Connection: keep-alive\r\n"
                "\r\n{}";
                std::string rspBuf = componet::format(pattern, body.size(), body);
                const auto& ret = net::HttpPacket::tryParse(net::HttpMsg::Type::response, rspBuf.data(), rspBuf.size());
                if (ret.first == nullptr)
                {
                    LOG_ERROR("ASS, ass response repack failed, rawdata={}, hcid={}", rspBuf, clihcid);
                    status = Status::assAbort;
                    conns.eraseConnection(asshcid);
                    break;
                }
                auto packet = ret.first;
                while (!conns.sendPacket(clihcid, packet))
                {
                    corot::this_corot::yield();
                    if (status != Status::rspToCli)
                        break;
                }

                LOG_TRACE("ASS, send response to cli, hcid={}", clihcid);
                status = Status::done;
            }
        case Status::done:
            {
                LOG_TRACE("ASS, done, destroy later,  hcid={}", clihcid);
//                conns.eraseConnection(clihcid);
            }
            return;
        case Status::assAbort:
            {
                //TODO 发送一个http404给cli
                std::string rspRaw =
                "HTTP/1.1 404 Not Found\r\n"
                "Server: Tengine/1.5.2\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: 13\r\n"
                "Connection: keep-alive\r\n"
                "\r\n404 Not Found";

                const auto& ret = net::HttpPacket::tryParse(net::HttpMsg::Type::response, rspRaw.data(), rspRaw.size());
                if (ret.first == nullptr)
                {
                    LOG_ERROR("ASS, ass response repack failed, rawdata={}, hcid={}", rspRaw, clihcid);
                }
                else
                {
                    auto packet = ret.first;
                    if (!conns.sendPacket(clihcid, packet))
                        corot::this_corot::yield();
                }
                conns.eraseConnection(clihcid);
                LOG_TRACE("ASS, assAbort, destroy later, hcid={}", clihcid);
            }
            return;
        case Status::cliAbort:
            {
                conns.eraseConnection(asshcid);
                LOG_TRACE("ASS, cliAbort, destroy later, hcid={}", clihcid);
            }
            return;
        default:
            return;
        }
    }
}

void AnySdkLoginManager::timerExec(componet::TimePoint now)
{
    m_now = now;

    //删除过期tokens
    for (auto iter = m_tokens.begin(); iter != m_tokens.end(); )
    {
        if (iter->second->expiry <= componet::toUnixTime(now))
        {
            iter = m_tokens.erase(iter);
            continue;
        }
        ++iter;
    }

    {//启动新进连接的处理协程
        std::lock_guard<componet::Spinlock> lock(m_allClients.newClentsLock);
        for (auto iter = m_allClients.newClients.begin(); iter != m_allClients.newClients.end(); )
        {
            auto client = iter->second;
            iter = m_allClients.newClients.erase(iter);
            
            if (!m_allClients.corotClients.insert({client->clihcid, client}).second)
            {
                LOG_ERROR("ASS, move client to corotClients failed, hcid={}", client->clihcid);
                Gateway::me().httpConnectionManager().eraseConnection(client->clihcid);
                continue;
            }

            //为这个连接启一个协程
            corot::create(std::bind(&AllClients::AnySdkClient::corotExec, client));
            LOG_TRACE("ASS, CREATE CROTO successed, hcid={}", client->clihcid);
        }
    }

    {//处理连接断开事件
        std::list<HttpConnectionId> closedHcids;
        {
            std::lock_guard<componet::Spinlock> lock(m_closedHcidsLock);
            closedHcids.swap(m_closedHcids);
        }
        std::lock_guard<componet::Spinlock> lock(m_allClients.newClentsLock);
        for (auto hcid : closedHcids)
        {
            HttpConnectionId clihcid = isCliHcid(hcid) ? hcid : hcid - 1;
            auto iter = m_allClients.corotClients.find(clihcid);
            if (iter == m_allClients.corotClients.end())
                continue;

            auto client = iter->second;

            //所有处理均已结束, 无所谓了
            if ( client->status >= AllClients::AnySdkClient::Status::done)
                continue;

            //client 断开, 且相关逻辑并未处理完成
            if ( (isCliHcid(hcid) && (client->status <= AllClients::AnySdkClient::Status::rspToCli)) )
            {
                client->status = AllClients::AnySdkClient::Status::cliAbort;
                continue;
            }

            //ass 断开, 且相关逻辑并未处理完成
            if(isAssHcid(hcid) && (client->status <= AllClients::AnySdkClient::Status::recvAssRsp))
            {
                client->status = AllClients::AnySdkClient::Status::assAbort;
                continue;
            }
        }
    }

    {//销毁失效的client
        for (auto iter = m_allClients.corotClients.begin(); iter != m_allClients.corotClients.end(); )
        {
            if (iter->second->status == AllClients::AnySdkClient::Status::destroy)
            {
                LOG_DEBUG("ASS destroy client obj, hcid={}", iter->second->clihcid);
                iter = m_allClients.corotClients.erase(iter);
                continue;
            }
            ++iter;
        }
    }

    {//从消息队列中取包并处理
        dealHttpPackets(now);
    }
}

}
