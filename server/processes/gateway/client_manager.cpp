#include "client_manager.h"

#include "gateway.h"
#include "anysdk_login_manager.h"
#include "game_config.h"

#include "componet/logger.h"
#include "componet/datetime.h"

#include "protocol/protobuf/proto_manager.h"
#include "protocol/protobuf/public/client.codedef.h"
#include "protocol/protobuf/private/login.codedef.h"

#include "dbadaptcher/dbconnection_pool.h"
#include "componet/http_request.h"
#include "componet/json.h"

namespace gateway{

using namespace protocol;
using namespace protobuf;





using LockGuard = std::lock_guard<componet::Spinlock>;

ClientManager::ClientManager(ProcessId pid)
: m_pid(pid),
  m_clientCounter(1)
{
}

ClientManager::Client::Ptr ClientManager::createNewClient()
{
    auto client = Client::create();
    uint64_t now = componet::toUnixTime(componet::Clock::now());
    uint32_t processNum = m_pid.num();
    uint32_t counter = m_clientCounter.fetch_add(1);
    client->ccid = (now << 32) | (processNum << 24) | counter;
    return client;
}

bool ClientManager::insert(Client::Ptr client)
{
    if (client == nullptr)
        return false;
    LockGuard lock(m_clientsLock);
    return m_clients.insert(client->ccid, client);
}

void ClientManager::erase(Client::Ptr client)
{
    if (client == nullptr)
        return;
    {
        LockGuard lock(m_clientsLock);
        auto iter = m_clients.find(client->ccid);
        if (iter == m_clients.end())
            return;

        LOG_TRACE("ClientManager erase client, cuid={}, ccid={}", client->cuid, client->ccid);
        m_clients.erase(client->ccid);
    }
    //通知业务进程
    PROTO_VAR_PRIVATE(ClientDisconnected, snd);
    snd.set_ccid(client->ccid);
    Gateway::me().sendToPrivate(ProcessId("lobby", 1), sndCode, snd);

    //删除事件
    e_afterEraseClient(client->ccid);
}

ClientManager::Client::Ptr ClientManager::getByCcid(ClientConnectionId ccid)
{
    LockGuard lock(m_clientsLock);
    auto iter = m_clients.find(ccid);
    if (iter == m_clients.end())
        return nullptr;
    return iter->second;
}

void ClientManager::eraseLater(Client::Ptr client)
{
    if (client == nullptr)
        return;
    LockGuard lock(m_dyingClientsLock);
    m_dyingClients.push_back(client);
}

void ClientManager::timerExec(const componet::TimePoint& now)
{
    { //action 1, 执行延迟删除
        LockGuard lock(m_dyingClientsLock);
        for (auto client : m_dyingClients)
            erase(client);

        m_dyingClients.clear();
    }

    {
        static componet::TimePoint lastTime;
        if (now - lastTime > std::chrono::seconds(10))
        {
            lastTime = now;
            LOG_TRACE("当前在线人数, {}", m_clients.size());
        }
    }
}

ClientConnectionId ClientManager::clientOnline(const net::Endpoint& ep)
{
    auto client = createNewClient();
    if (!insert(client))
    {
        LOG_ERROR("ClientManager::clientOnline failed, 生成的ccid出现重复, ccid={}", client->ccid);
        return INVALID_CCID;
    }
    client->ep = ep;

    LOG_TRACE("客户端接入，分配ClientConnectionId, ccid={}, ep={}", client->ccid, ep);
    return client->ccid;
}

void ClientManager::clientOffline(ClientConnectionId ccid)
{
    Client::Ptr client = getByCcid(ccid);
    if (client == nullptr)
    {
        LOG_TRACE("ClientManager, 客户端离线, ccid不存在, ccid={}", ccid);
        return;
    }

    if(client->state == Client::State::logining)
        LOG_TRACE("ClientManager, 客户端离线, 登录过程终止, ccid={}", ccid);
    else
        LOG_TRACE("ClientManager, 客户端离线, ccid={}", ccid);

    //destroy
    eraseLater(client);
}

void ClientManager::kickOutClient(ClientConnectionId ccid, bool delay/* = true*/)
{
    Client::Ptr client = getByCcid(ccid);

    if (client == nullptr)
    {
        LOG_TRACE("ClientManager, 踢下线, ccid不存在, ccid={}, delay={}", ccid, delay);
        return;
    }

    if(client->state == Client::State::logining)
        LOG_TRACE("ClientManager, 踢下线, 登录过程终止, ccid={}, delay={}", ccid, delay);
    else
        LOG_TRACE("ClientManager, 踢下线, ccid={}, delay={}", ccid, delay);
    //TODO 这里要加入到客户端的通知， 告知为何踢出, 次函数需要修改加入一个踢出原因参数
    delay ? eraseLater(client) : erase(client);
    return;
}

void ClientManager::sendServerVisionToClient(ClientConnectionId ccid) const
{
    const auto& versionCfg = GameConfig::me().data().versionInfo;
    LOG_DEBUG("send server version to client, version={}, appleReview={}", versionCfg.version, versionCfg.appleReview);

    PROTO_VAR_PUBLIC(S_ServerVersion, snd);
    snd.set_version(versionCfg.version);
    snd.set_apple_review(versionCfg.appleReview);
    snd.set_strict_version(versionCfg.strictVersion);
    snd.set_ios_app_url(versionCfg.iosAppUrl);
    snd.set_android_app_url(versionCfg.androidAppUrl);
    Gateway::me().sendToClient(ccid, sndCode, snd);

    sendSystemNoticeToClient(ccid);
}

void ClientManager::sendSystemNoticeToClient(ClientConnectionId ccid) const
{
    PROTO_VAR_PUBLIC(S_Notice, snd1);
    snd1.set_type(PublicProto::S_Notice::NOTICE);
    snd1.set_text(GameConfig::me().data().systemNotice.announcementBoard);
    Gateway::me().sendToClient(ccid, snd1Code, snd1);

    PROTO_VAR_PUBLIC(S_Notice, snd2);
    snd2.set_type(PublicProto::S_Notice::MARQUEE);
    snd2.set_text(GameConfig::me().data().systemNotice.marquee);
    Gateway::me().sendToClient(ccid, snd2Code, snd2);

    return;
}

void ClientManager::sendSystemNoticeToAllClients() const
{
    PROTO_VAR_PUBLIC(S_Notice, snd1);
    snd1.set_type(PublicProto::S_Notice::NOTICE);
    snd1.set_text(GameConfig::me().data().systemNotice.announcementBoard);

    PROTO_VAR_PUBLIC(S_Notice, snd2);
    snd2.set_type(PublicProto::S_Notice::MARQUEE);
    snd2.set_text(GameConfig::me().data().systemNotice.marquee);

    for (auto it = m_clients.begin(); it != m_clients.end(); ++it)
    {
        const auto ccid = it->first;
        Gateway::me().sendToClient(ccid, snd1Code, snd1);
        Gateway::me().sendToClient(ccid, snd2Code, snd2);
    }
}

void ClientManager::relayClientMsgToServer(const ProcessId& pid, TcpMsgCode code, const ProtoMsgPtr& protoPtr, ClientConnectionId ccid)
{
    LOG_DEBUG("relay client msg to process {}, code={}", pid, code);
    Gateway::me().relayToPrivate(ccid, pid, code, *protoPtr);
}

void ClientManager::relayClientMsgToClient(TcpMsgCode code, const ProtoMsgPtr& protoPtr, ClientConnectionId ccid)
{
    LOG_DEBUG("relay client msg to client {}, code={}", ccid, code);
    Gateway::me().sendToClient(ccid, code, *protoPtr);
}

void ClientManager::proto_C_Login(ProtoMsgPtr proto, ClientConnectionId ccid)
{
    auto rcv = PROTO_PTR_CAST_PUBLIC(C_Login, proto);


    auto client = getByCcid(ccid);
    if (client == nullptr)
    {
        LOG_DEBUG("login, step 1, 处理 C_Login, client 已经离线, ccid={}, openid={}", ccid, rcv->openid());
        return;
    }

    PROTO_VAR_PRIVATE(LoginQuest, toserver);
    toserver.set_openid(rcv->openid());
    toserver.set_token(rcv->token());
    toserver.set_ccid(ccid);
    toserver.set_name(rcv->nick_name());
    toserver.set_imgurl(rcv->imgurl());
    toserver.set_ipstr(client->ep.ip.toString());
    toserver.set_sex(rcv->sex());

    std::map<std::string, std::string> wechatInfo;

    auto loginFailed = [this, &client, &rcv, ccid](int errCode)
    {
        PROTO_VAR_PUBLIC(S_LoginRet, tocli)
        tocli.set_ret_code(errCode);
        Gateway::me().sendToClient(client->ccid, tocliCode, tocli);
        LOG_DEBUG("login, step 1, 验证失败, type={}, ccid={}, openid={}, token={}", rcv->login_type(), ccid, rcv->openid(), rcv->token());
        eraseLater(client);
    };

    //TODO login step1, 依据登陆类型和登陆信息验证登陆有效性, 微信的
    if (rcv->login_type() == PublicProto::LOGINT_WETCHAT)
    {
        //if (!AnySdkLoginManager::me().checkAccessToken(rcv->openid(), rcv->token()))
        //{
        //    loginFailed(PublicProto::LOGINR_WCHTTOKEN_ILEGAL);
        //    return;
        //}
        if(checkWechatLogin(rcv->os(), rcv->token(), wechatInfo) != 0)
        {
            loginFailed(PublicProto::LOGINR_WCHTTOKEN_ILEGAL);
            return;
        }
        toserver.set_verified(true);
    }
    else if(rcv->login_type() == PublicProto::LOGINT_VISTOR)
    {
        if (!GameConfig::me().data().versionInfo.appleReview)
        {
            loginFailed(PublicProto::LOGINR_FAILED);
            return;
        }
        toserver.set_verified(true);
    }
    else if(rcv->login_type() == PublicProto::LOGINT_HISTOLKEN)
    {
        {//屏蔽掉历史纪录登录, 这里直接登陆失败
            loginFailed(PublicProto::LOGINR_FAILED);
            return; 
        }
        toserver.set_verified(false);
    }

    if(wechatInfo.size() > 0)
    {
        //TODO openid,看需不需要加前缀
        toserver.set_openid(wechatInfo["openid"]);
        toserver.set_name(wechatInfo["nickname"]);
        toserver.set_imgurl(wechatInfo["img"]);
        toserver.set_sex(std::atoi(wechatInfo["sex"].c_str()));
    }
    
    Gateway::me().sendToPrivate(ProcessId("lobby", 1), toserverCode, toserver);
    LOG_TRACE("login, step 1, 验证通过, type={}, ccid={}, openid={}, token={}", rcv->login_type(), ccid, rcv->openid(), rcv->token());
}

int ClientManager::checkWechatLogin(std::string os, std::string code, std::map<std::string, std::string>& info)
{
	char ios_check_url[] = "127.0.0.1:81/wechat.php";
    componet::HttpRequest http;
	char http_return[1024] = {0};

	char params[1024] = "";
    snprintf(params, sizeof(params), "os=%s&code=%s", os.c_str(), code.c_str());

	if(http.HttpPost(ios_check_url, params, http_return)){
		std::string ret = http_return;
		std::size_t idx = ret.find("{");
		if(idx != std::string::npos)
		{
			std::size_t end = ret.find_last_of("}");
			if(end != std::string::npos)
			{
                LOG_TRACE("content {}", ret.c_str());
				//std::cout << ret.substr(idx, end - idx + 1) << "\n------------------\n" << std::endl;
				std::string jsonObj = ret.substr(idx, end - idx + 1);
                auto j = componet::json::parse(jsonObj);
                if(!j["msg"].is_null())
                {
                    return -1;
                }
                if(!j["openid"].is_null())
                {
                    info["nickname"] = j["nickname"].get<std::string>();
                    info["sex"] = j["sex"].get<int>();
                    info["img"] = j["headimgurl"].get<std::string>();
                    info["openid"] = j["openid"].get<std::string>();
                    return 0;
                }
			}
		}
	}
	return -1;
}

void ClientManager::proto_C_LoginOut(ProtoMsgPtr proto, ClientConnectionId ccid)
{
    auto rcv = PROTO_PTR_CAST_PUBLIC(C_LoginOut, proto);

    auto client = getByCcid(ccid);
    if (client == nullptr)
    {
        LOG_DEBUG("login out, step 1, 处理 C_LoginOut, client 已经离线, ccid={}", ccid);
        return;
    }
    kickOutClient(ccid, true);
}

void ClientManager::proto_RetLoginQuest(ProtoMsgPtr proto)
{
    auto rcv = PROTO_PTR_CAST_PRIVATE(RetLoginQuest, proto);
    auto client = getByCcid(rcv->ccid());
    if (client == nullptr)
    {
        LOG_TRACE("login, step 3, lobby 返回结果, client 已离线, ccid={}, openid={}, cuid={}", rcv->ccid(), rcv->openid(), rcv->cuid());
        return;
    }

    client->cuid = rcv->cuid();
    client->state = Client::State::playing;

    PROTO_VAR_PUBLIC(S_LoginRet, snd);
    int32_t rc = rcv->ret_code();
    if (rc == PrivateProto::RLQ_SUCCES)
    {
        snd.set_ret_code(PublicProto::LOGINR_SUCCES);
        snd.set_cuid(client->cuid);
        snd.set_temp_token("xxxx"); //此版本一律返回xxxx， 此字段暂不启用
        snd.set_ipstr(client->ep.ip.toString());

        const auto& customServiceCfg = GameConfig::me().data().customService;
        snd.set_wechat1(customServiceCfg.wechat1);
        snd.set_wechat2(customServiceCfg.wechat2);
        snd.set_wechat3(customServiceCfg.wechat3);
        snd.set_share_link(customServiceCfg.shareLink);

        snd.set_nick_name(rcv->nick_name());
        snd.set_imgurl(rcv->imgurl());
        snd.set_sex(rcv->sex());

        const auto& wechatShareCfg = GameConfig::me().data().wechatShare;
        snd.set_wechat_share_title(wechatShareCfg.title);
        snd.set_wechat_share_content(wechatShareCfg.content);

        const auto& priceCfg = GameConfig::me().data().pricePerPlayer;
        for (const auto& item : priceCfg)
        {
            auto price = snd.add_price_list();
            price->set_rounds(item.first);
            price->set_money(item.second);
        }
        const auto& gameListCfg = GameConfig::me().data().gameList;
        for (const auto& item : gameListCfg)
        {
            auto game = snd.add_game_list();
            game->set_gameid(item.first);
            game->set_goldmode(item.second.goldMode);
            game->set_cardmode(item.second.cardMode);
            game->set_url(item.second.url);
            game->set_version(item.second.version);
        }

        //snd.set_max_player_size(GameConfig::me().data().roomPropertyLimits.maxPlayerSize);
        snd.set_max_player_size(rcv->roomid());
        LOG_TRACE("login, step 3, 登陆成功, ccid={}, openid={}, cuid={}", rcv->ccid(), rcv->openid(), rcv->cuid());

        //char sql[100] = {0};
        //snprintf(sql, sizeof(sql), "insert into client_data values('%s', %ld, %d, %d, %d, null);", rcv->openid().c_str(), rcv->cuid(), 188, 100, 0);
        //dbadaptcher::MysqlConnectionPool::me().insert(sql);
    }
    else
    {
        LOG_TRACE("login, step 3, 登陆失败, code={}, ccid={}, openid={}, cuid={}", rc, rcv->ccid(), rcv->openid(), rcv->cuid());
        if (rc == PrivateProto::RLQ_TOKEN_EXPIRIED)
            snd.set_ret_code(PublicProto::LOGINR_HISTOKEN_ILEGAL);
        else //if (rc == PrivateProto::RLQ_FAILED)
            snd.set_ret_code(PublicProto::LOGINR_FAILED);
        //eraseLater(client); //登陆失败不删连接, 让端自己断掉即可
        dbadaptcher::MysqlConnectionPool::me().getconn();
    }
    Gateway::me().sendToClient(client->ccid, sndCode, snd);
}

void ClientManager::proto_ClientBeReplaced(ProtoMsgPtr proto)
{
    auto rcv = PROTO_PTR_CAST_PRIVATE(ClientBeReplaced, proto);
    auto client = getByCcid(rcv->ccid());
    if (client == nullptr)
    {
        LOG_TRACE("login, step 2.5, 客户端被挤下线, 已经不在线了, ccid={}, openid={}, cuid={}", rcv->ccid(), rcv->openid(), rcv->cuid());
        return;
    }
    
    LOG_TRACE("login, step 2.5, 客户端被挤下线, ccid={}, openid={}, cuid={}", rcv->ccid(), rcv->openid(), rcv->cuid());
    kickOutClient(rcv->ccid());
}

void ClientManager::regMsgHandler()
{
    using namespace std::placeholders;
    /**********************msg from client*************/
    REG_PROTO_PUBLIC(C_Login, std::bind(&ClientManager::proto_C_Login, this, _1, _2));
    auto heartbeat = [](ProtoMsgPtr rcv, ClientConnectionId ccid) { Gateway::me().sendToClient(ccid, PROTO_CODE_PUBLIC(CS_Heartbeat), *rcv);};
    REG_PROTO_PUBLIC(CS_Heartbeat, std::bind(heartbeat, _1, _2));
    REG_PROTO_PUBLIC(C_LoginOut, std::bind(&ClientManager::proto_C_LoginOut, this, _1, _2));

    /*********************msg from cluster*************/
    REG_PROTO_PRIVATE(RetLoginQuest, std::bind(&ClientManager::proto_RetLoginQuest, this, _1));
    REG_PROTO_PRIVATE(ClientBeReplaced, std::bind(&ClientManager::proto_ClientBeReplaced, this, _1));
}


}

