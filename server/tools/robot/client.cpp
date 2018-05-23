#include "client.h"

#include "componet/scope_guard.h"
#include "coroutine/coroutine.h"

#include "protocol/protobuf/public/client.codedef.h"

Client& Client::me()
{
    return s_me;
}

Client Client::s_me;


Client::Client()
    : m_recvPacket(net::TcpPacket::create()), m_serverEp(serverAddr)
{
}

void Client::run()
{
    try 
    {
        ProtoManager::me().loadConfig(cfgDir);
        registerRobotMsg();

        m_connList.reserve(robotNum);
        m_recvQueList.resize(robotNum);
        for(int i = 0; i < robotNum; ++i)
        {
            auto tcpConn = net::TcpConnection::connect(m_serverEp);
            m_connList[i] = net::BufferedConnection::create(std::move(*tcpConn));
            m_connList[i]->setNonBlocking();
        }

        //auto tcpConn = net::TcpConnection::connect(m_serverEp);
        //m_conn = net::BufferedConnection::create(std::move(*tcpConn));
        //m_conn->setNonBlocking();
        m_epoller.setEventHandler(std::bind(&Client::epollEventHandler, this, std::placeholders::_3));

        for(int i = 0;i < robotNum; ++i)
        {
            m_epoller.regSocket(m_connList[i]->getFD(), net::Epoller::Event::read);
        }
        //m_epoller.regSocket(m_conn->getFD(), net::Epoller::Event::read);
        while(true)
        {
            m_epoller.wait(std::chrono::milliseconds(3));
            m_timer();
            corot::schedule();
        }
    }
    catch (const net::ReadClosedConnection& ex)
    {
        for(int i = 0; i < robotNum; ++i)
        {
            m_epoller.delSocket(m_connList[i]->getFD());
        }
        //m_epoller.delSocket(m_conn->getFD());
        LOG_TRACE("server closed the connection");
        return;
    }
    catch(const net::NetException& ex)
    {
        LOG_ERROR("exception occured，ex={}", ex);
        return;
    }
}

void Client::epollEventHandler(net::Epoller::Event event)
{
        switch (event)
        {
        case net::Epoller::Event::read:
            {
                net::TcpPacket::Ptr& packet = m_recvPacket;
                for(int i = 0; i < robotNum; ++i)
                {
                    while(m_connList[i]->tryRecv())
                    {
                        const auto& rawBuf = m_connList[i]->recvBuf().readable();
                        auto readSize = packet->parse(rawBuf.first, rawBuf.second);
                        m_connList[i]->recvBuf().commitRead(readSize);
                        if (!packet->complete())
                            break;
                        if (!m_recvQueList[i].push(packet))
                            break;
                        packet = net::TcpPacket::create();
                    }
                }

                //net::TcpPacket::Ptr& packet = m_recvPacket;
                //while(m_conn->tryRecv())
                //{
                //    const auto& rawBuf = m_conn->recvBuf().readable();
                //    auto readSize = packet->parse(rawBuf.first, rawBuf.second);
                //    m_conn->recvBuf().commitRead(readSize);
                //    if (!packet->complete())
                //        break;
                //    if (!m_recvQue.push(packet))
                //        break;
                //    packet = net::TcpPacket::create();
                //}
            }
            break;
        case net::Epoller::Event::write:
            {
                for(int i = 0; i < robotNum; ++i)
                {
                    m_connList[i]->trySend();
                }
                //m_conn->trySend();
//                while(m_conn->trySend())
//                {
//                    if(m_sendQue.empty())
//                    {
//                        m_epoller.modifySocket(m_conn->getFD(), net::Epoller::Event::read);
//                        break;
//                    }
//                    m_conn->setSendPacket(m_sendQue.top());
//                    m_sendQue.pop();
//                }
            }
            break;
        case net::Epoller::Event::error:
            {
            }
            break;
        default:
            break;
        }
    return;
}

void Client::dealMsg(const componet::TimePoint& now, uint32_t idx)
{
    while (!m_recvQueList[idx].empty())
    {
        auto packet = std::static_pointer_cast<net::TcpPacket>(m_recvQueList[idx].top());
        m_recvQueList[idx].pop();
        auto msg = reinterpret_cast<process::TcpMsg*>(packet->content());
        ProtoManager::me().dealTcpMsg(msg, packet->contentSize(), 0, now);
    }
}

bool Client::sendMsg(TcpMsgCode msgCode, const ProtoMsg& proto, uint32_t idx)
{
    return trySendMsg(msgCode, proto, idx);
}

ProtoMsgPtr Client::recvMsg(TcpMsgCode msgCode, int32_t* idx)
{
    //auto msg = tryRecvMsg(msgCode, idx);
    //if (msg != nullptr)
    //    return msg;

    while(true)
    {
        for(int32_t i = 0; i < robotNum; ++i)
        {
            auto msg = tryRecvMsg(msgCode, i);
            if (msg != nullptr)
            {
                *idx = i;
                return msg;
            }
        }
        corot::this_corot::yield();
    }
    return nullptr; //never reach here
}

bool Client::trySendMsg(TcpMsgCode msgCode, const ProtoMsg& proto, uint32_t idx)
{
    while(!m_connList[idx]->trySend())
        corot::this_corot::yield();

	//create packet
    const uint32_t protoBinSize = proto.ByteSize();
    const uint32_t bufSize = sizeof(process::TcpMsg) + protoBinSize;
    uint8_t* buf = new uint8_t[bufSize];
    ON_EXIT_SCOPE_DO(delete[] buf);

    process::TcpMsg* msg = new(buf) process::TcpMsg(msgCode);
    if(!proto.SerializeToArray(msg->data, protoBinSize))
    {   
        LOG_ERROR("proto serialize failed, msgCode = {}", msgCode);
        return false;
    }   

    auto packet = net::TcpPacket::create();
    packet->setContent(buf, bufSize);
        
    while (packet->size() > 0)
    {
        auto rawBuf = m_connList[idx]->sendBuf().writeable();
        auto copySize = packet->copy(rawBuf.first, rawBuf.second);
        m_connList[idx]->sendBuf().commitWrite(copySize);
        packet->pop(copySize);
        m_connList[idx]->trySend();
        corot::this_corot::yield();
    }
    while(!m_connList[idx]->trySend())
        corot::this_corot::yield();
    return true;
}

ProtoMsgPtr Client::tryRecvMsg(TcpMsgCode msgCode, uint32_t idx)
{
    if (m_recvQueList[idx].empty())
        return nullptr;
    auto packet = std::static_pointer_cast<net::TcpPacket>(m_recvQueList[idx].top());
    auto raw = reinterpret_cast<process::TcpMsg*>(packet->content());
    auto iter = std::find(msgList.begin(), msgList.end(), raw->code);
    if(iter == msgList.end())
    {
        LOG_TRACE("proto skip msgCode: {}", raw->code);
        m_recvQueList[idx].pop();
        return nullptr;
    }
    if (raw->code != msgCode)
        return nullptr;
    m_recvQueList[idx].pop();
    const uint8_t* data = raw->data;
    ProtoMsgPtr ret = ProtoManager::me().create(msgCode);
    ret->ParseFromArray(data, packet->contentSize());
    return ret;
}

void Client::registerRobotMsg()
{
    msgList.push_back(PROTO_CODE_PUBLIC(S_Notice));
    msgList.push_back(PROTO_CODE_PUBLIC(S_ServerVersion));
    msgList.push_back(PROTO_CODE_PUBLIC(S_G13_PlayersInRoom));
    msgList.push_back(PROTO_CODE_PUBLIC(S_G13_HandOfMine));
    msgList.push_back(PROTO_CODE_PUBLIC(S_Chat));
    msgList.push_back(PROTO_CODE_PUBLIC(S_LoginRet));
    msgList.push_back(PROTO_CODE_PUBLIC(S_G13_RoomAttr));
    msgList.push_back(PROTO_CODE_PUBLIC(S_G13_HandOfMine));
    msgList.push_back(PROTO_CODE_PUBLIC(S_G13_AllHands));
}


