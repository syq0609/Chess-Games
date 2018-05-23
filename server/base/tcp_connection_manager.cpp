#include "tcp_connection_manager.h"

#include "process.h"
#include "tcp_message.h"

#include "componet/other_tools.h"
#include "componet/logger.h"
#include "componet/string_kit.h"

#include <iostream>
#include <mutex>
#include <thread>

namespace water{
namespace process{


void TcpConnectionManager::addPrivateConnection(net::BufferedConnection::Ptr conn, 
                                                   ProcessId processId)
{
    conn->setNonBlocking();

    auto connHolder = std::make_shared<ConnectionHolder>();
    connHolder->type = ConnType::privateType;
    connHolder->id = processId.value();
    connHolder->conn = conn;

    std::lock_guard<componet::Spinlock> lock(m_lock);

    auto insertAllRet = m_allConns.insert({conn->getFD(), connHolder});
    if(insertAllRet.second == false)
    {
        LOG_ERROR("ConnectionManager, insert privateConn to m_allConns failed, processId={}", processId);
        return;
    }

    auto& privateProcessByType = m_privateConns[processId.type()];

    auto insertPrivateRet = privateProcessByType.insert({processId.num(), connHolder});
    if(insertPrivateRet.second == false)
    {
        LOG_ERROR("ConnectionManager, insert privateConn to m_privateConns failed, processId={}", processId);
        m_allConns.erase(insertAllRet.first);
        return;
    }

    try
    {
        m_epoller.regSocket(conn->getFD(), net::Epoller::Event::read);
        connHolder->recvPacket = net::TcpPacket::create();
    }
    catch (const componet::ExceptionBase& ex)
    {
        LOG_ERROR("ConnectionManager, insert privateConn to epoller failed, ex={}, processId={}",
                  ex.what(), processId);
        m_allConns.erase(insertAllRet.first);
        privateProcessByType.erase(insertPrivateRet.first);
    }

    //添加privateConn完成的事件
    e_afterAddPrivateConn(processId);
    LOG_TRACE("ConnectionManager, insert privateConn, fd={}, pid={}, ep={}", 
              conn->getFD(), processId, conn->getRemoteEndpoint());
}

bool TcpConnectionManager::addPublicConnection(net::BufferedConnection::Ptr conn, ClientSessionId clientId)
{
    if (conn == nullptr)
        return false;

    conn->setNonBlocking();
    auto connHolder = std::make_shared<ConnectionHolder>();
    connHolder->type = ConnType::publicType;
    connHolder->id = clientId;
    connHolder->conn = conn;

    std::lock_guard<componet::Spinlock> lock(m_lock);

    auto insertAllRet = m_allConns.insert({conn->getFD(), connHolder});
    if(insertAllRet.second == false)
    {
        LOG_ERROR("ConnectionManager::addPublicConnection failed, clientId={}, ep={}", clientId, conn->getRemoteEndpoint());
        return false;
    }
   
    auto insertPublicRet = m_publicConns.insert({clientId, connHolder});
    if(insertPublicRet.second == false)
    {
        LOG_ERROR("ConnectionManager::addPublicConnection failed, clientId={}, ep={}", clientId, conn->getRemoteEndpoint());
        m_allConns.erase(insertAllRet.first);
        return false;
    }
    
    try
    {
        m_epoller.regSocket(conn->getFD(), net::Epoller::Event::read);
        connHolder->recvPacket = net::TcpPacket::create();
    }
    catch (const componet::ExceptionBase& ex)
    {
        LOG_ERROR("ConnectionManager::addPublicConnection failed, {}, clientId={}, ep={}", 
                  ex, clientId, conn->getRemoteEndpoint());

        m_allConns.erase(insertAllRet.first);
        m_publicConns.erase(insertPublicRet.first);
        return false;
    }
    LOG_TRACE("ConnectionManager, insert publicConn, fd={}, id={}, ep={}", conn->getFD(), clientId, conn->getRemoteEndpoint());
    return true;
}

net::BufferedConnection::Ptr TcpConnectionManager::erasePublicConnection(ClientSessionId clientId)
{
    net::BufferedConnection::Ptr ret;

    {//局部锁, 否则调用eraseConnection会死锁
        std::lock_guard<componet::Spinlock> lock(m_lock);
        auto publicIt = m_publicConns.find(clientId);
        if(publicIt == m_publicConns.end())
            return nullptr;
        ret = publicIt->second->conn;
    }

    eraseConnection(ret);
    return ret;
}

void TcpConnectionManager::eraseConnection(net::BufferedConnection::Ptr conn)
{
    bool publicConnErased = false;
    ClientSessionId erasedPublicConnId = 0;
    bool privateConnErased = false;
    ProcessId erasedPrivateConnId = 0;

    {
        std::lock_guard<componet::Spinlock> lock(m_lock);

        auto it = m_allConns.find(conn->getFD());
        if(it == m_allConns.end())
            return;

        auto publicIt = m_publicConns.find(it->second->id);
        if(publicIt != m_publicConns.end())
        {
            LOG_TRACE("ConnectionManager, erase publicConn, id={}", it->second->id);
            m_publicConns.erase(publicIt);
            publicConnErased = true;
            erasedPublicConnId = it->second->id;
        }
        else
        {
            ProcessId pid(it->second->id);
            auto iter = m_privateConns.find(pid.type());
            if(iter != m_privateConns.end() && iter->second.find(pid.num()) != iter->second.end())
            {
                LOG_TRACE("ConnectionManager, erase privateConn, id={}, fd={}", pid, conn->getFD());
                iter->second.erase(pid.num());
                privateConnErased = true;
                erasedPrivateConnId = pid;
            }
            else
            {
                LOG_ERROR("ConnectionManager, erase conn, notPublic and notPrivate id={}", pid);
            }
        }
        LOG_TRACE("ConnectionManager, erase conn, fd={}, id={}, ep={}", conn->getFD(), it->second->id, conn->getRemoteEndpoint());
        m_epoller.delSocket(conn->getFD());
        m_allConns.erase(it);
    }

    if (publicConnErased)
        e_afterErasePublicConn(erasedPublicConnId);
    else if (privateConnErased)
        e_afterErasePrivateConn(erasedPrivateConnId);

}

bool TcpConnectionManager::exec()
{
    try
    {
        if(!checkSwitch())
            return true;

        //绑定epoll消息处理器
        using namespace std::placeholders;
        m_epoller.setEventHandler(std::bind(&TcpConnectionManager::epollerEventHandler, this, _2, _3));

        while(checkSwitch())
        {
            m_epoller.wait(std::chrono::milliseconds(5)); //5 milliseconds 一轮
        }
    }
    catch (const net::NetException& ex)
    {
        LOG_ERROR("TcpConnManager 异常退出 , {}", ex.what());
        return false;
    }

    return true;
}

void TcpConnectionManager::epollerEventHandler(int32_t socketFD, net::Epoller::Event event)
{
    ConnectionHolder::Ptr connHolder;
    {
        std::lock_guard<componet::Spinlock> lock(m_lock);
        auto connsIter = m_allConns.find(socketFD);
        if(connsIter == m_allConns.end()) 
        {
            if(net::Epoller::Event::error != event)
            {
                LOG_ERROR("epoll报告一个socket事件，但该socket不在manager中, fd={}, event={}", socketFD, event);
            }
            m_epoller.delSocket(socketFD);
            return;
        }
        connHolder = connsIter->second;
    }
    net::BufferedConnection::Ptr conn = connHolder->conn;
    try
    {
        switch (event)
        {
        case net::Epoller::Event::read:
            {
                if (m_recvQueue.full()) //队列满, 不能收
                    break;
                while(conn->tryRecv())
                {
                    net::TcpPacket::Ptr packet = connHolder->recvPacket;
                    const auto& rawBuf = conn->recvBuf().readable();
                    const auto readSize = packet->parse(rawBuf.first, rawBuf.second);
//                    LOG_DEBUG("SOCKT DEBUG, PARSE, packetSize={}, contentSize={}, ep={}",  packet->size(), packet->contentSize(), conn->getRemoteEndpoint());
                    conn->recvBuf().commitRead(readSize);
                    if (!packet->complete())
                        break;
                    m_recvQueue.push({connHolder, packet});

//                   net::TcpPacket* tp = (net::TcpPacket*)packet.get();
//                   auto rawMsg = reinterpret_cast<water::process::TcpMsg*>(tp->content());
//                   if(water::process::isEnvelopeMsgCode(rawMsg->code))
//                   {
//                       auto envelope = reinterpret_cast<water::process::Envelope*>(tp->content());
//                       LOG_DEBUG("SOCKT DEBUG, RECV, packetsize={}, contentSize={}, msgCode={}, {}", packet->size(), tp->contentSize(), envelope->msg.code,
//                                 componet::dataToHex(packet->data(), packet->size()));
//                   }
//                   else
//                   {
//                       LOG_DEBUG("SOCKT DEBUG, RECV, packetsize={}, contentSize={}, msgCode={}, {}", packet->size(), tp->contentSize(), rawMsg->code, 
//                                 componet::dataToHex(packet->data(), packet->size()));
//                   }

                    connHolder->recvPacket = net::TcpPacket::create();
                }
            }
            break;
        case net::Epoller::Event::write:
            {
                std::lock_guard<componet::Spinlock> lock(connHolder->sendLock);
                while (connHolder->conn->trySend()) //缓冲区已经发空
                {
                    if (connHolder->sendQueue.empty()) //累积未发送的消息队列也已经发空
                    {
                        m_epoller.modifySocket(socketFD, net::Epoller::Event::read);
                        break;
                    }

                    while (!connHolder->sendQueue.empty())
                    {
                        net::Packet::Ptr packet = connHolder->sendQueue.front();
                        const auto& rawBuf = conn->sendBuf().writeable(packet->size());
                        if (rawBuf.second == 0)
                            break;

                        const auto copySize = packet->copy(rawBuf.first, rawBuf.second);
                        conn->sendBuf().commitWrite(copySize);
                        if (copySize < packet->size())
                        {
                            packet->pop(copySize);
                            break;
                        }
                        connHolder->sendQueue.pop_front();
                    }
                }
            }
            break;
        case net::Epoller::Event::error:
            {
                LOG_ERROR("epoll error, {}", conn->getRemoteEndpoint());
                eraseConnection(conn);
                conn->close();
            }
            break;
        default:
            LOG_ERROR("epoll, unexcept event type, {}", conn->getRemoteEndpoint());
            break;
        }
    }
    catch (const net::ReadClosedConnection& ex)
    {
        LOG_TRACE("对方断开连接, {}, fd={}", conn->getRemoteEndpoint(), conn->getFD());
        eraseConnection(conn);
        conn->close();
    }
    catch (const net::NetException& ex)
    {
        LOG_TRACE("连接异常, {}, {}", conn->getRemoteEndpoint(), ex.what());
        eraseConnection(conn);
        conn->close();
    }
}

bool TcpConnectionManager::getPacket(ConnectionHolder::Ptr* conn, net::Packet::Ptr* packet)
{
    std::pair<ConnectionHolder::Ptr, net::Packet::Ptr> ret;
    if(!m_recvQueue.pop(&ret))
        return false;

    *conn = ret.first;
    *packet = ret.second;
    return true;
}

bool TcpConnectionManager::sendPacketToPrivate(ProcessId processId, net::Packet::Ptr packet)
{
    std::lock_guard<componet::Spinlock> lock(m_lock);
    auto typeIter = m_privateConns.find(processId.type());
    if(typeIter == m_privateConns.end())
    {
        LOG_ERROR("send private packet, 目标进程未连接, processId={}", processId);
        return false;
    }

    auto numIter = typeIter->second.find(processId.num());
    if(numIter == typeIter->second.end())
    {
        LOG_ERROR("send private packet, 目标进程未连接, processId={}", processId);
        return false;
    }

    try
    {
        sendPacket(numIter->second, packet);
    }
    catch (const componet::ExceptionBase& ex)
    {
        LOG_ERROR("send private packet, socket异常, {}, remoteProcessId={}", ex.what(), processId);
        return false;
    }

    return true;
}

void TcpConnectionManager::broadcastPacketToPrivate(ProcessType processType, net::Packet::Ptr packet)
{
    std::lock_guard<componet::Spinlock> lock(m_lock);
    auto iter = m_privateConns.find(processType);
    if(iter == m_privateConns.end())
    {
        LOG_ERROR("broadcast private packet, 目标进程类型未连接, processType={}",
                  processType);
        return;
    }

    for(auto& item : iter->second)
    {
        try
        {
            sendPacket(item.second, packet);
        }
        catch (const componet::ExceptionBase& ex)
        {
            LOG_ERROR("broadcast private packet, socket异常, {}, remoteProcessId={}", 
                      ex.what(), ProcessId(item.second->id));
        }
    }
}

bool TcpConnectionManager::sendPacketToPublic(ClientSessionId clientId, net::Packet::Ptr packet)
{
    std::lock_guard<componet::Spinlock> lock(m_lock);
    auto it = m_publicConns.find(clientId);
    if(it == m_publicConns.end())
    {
        LOG_ERROR("send packet to public, socket not found, clientId={}", clientId);
        return false;
    }

    try
    {
        sendPacket(it->second, packet);
    }
    catch (const componet::ExceptionBase& ex)
    {
        LOG_ERROR("send private packet, socket异常, {}, id={}", ex.what(), clientId);
        return false;
    }
    return true;
}

void TcpConnectionManager::broadcastPacketToPublic(net::Packet::Ptr packet)
{
    std::lock_guard<componet::Spinlock> lock(m_lock);
    for(auto& item : m_publicConns)
    {
        try
        {
            sendPacket(item.second, packet);
        }
        catch (const componet::ExceptionBase& ex)
        {
            LOG_ERROR("broadcast private packet, socket异常, {}, id={}", ex, item.first);
        }
    }
}

bool TcpConnectionManager::sendPacket(ConnectionHolder::Ptr connHolder, net::Packet::Ptr packet)
{
   // std::this_thread::sleep_for(std::chrono::seconds(5));
   // LOG_DEBUG("SOCKT DEBUG, AFTER SLEEP, ----------------------------------------------------");
   // LOG_DEBUG("SOCKT DEBUG, PACKETDATA={}", componet::dataToHex(packet->data(), packet->size()));


    std::lock_guard<componet::Spinlock> lock(connHolder->sendLock);


    //net::TcpPacket* tp = (net::TcpPacket*)packet.get();
    //auto rawMsg = reinterpret_cast<water::process::TcpMsg*>(tp->content());
    //if(water::process::isEnvelopeMsgCode(rawMsg->code))
    //{
    //    auto envelope = reinterpret_cast<water::process::Envelope*>(tp->content());
    //    LOG_DEBUG("SOCKT DEBUG, SEND, PACKETSIZE={}, contentSize={}, msgCode={}", packet->size(), tp->contentSize(), envelope->msg.code);
    //}
    //else
    //{
    //    LOG_DEBUG("SOCKT DEBUG, SEND, PACKETSIZE={}, contentSize={}, msgCode={}", packet->size(), tp->contentSize(), rawMsg->code);
    //}

    if(!connHolder->sendQueue.empty()) //待发送队列非空, 直接排队
    {
        if(connHolder->sendQueue.size() > 512)
            return false;

        connHolder->sendQueue.push_back(packet);
        return true;
    }

    net::BufferedConnection::Ptr conn = connHolder->conn;
    const auto& rawBuf = conn->sendBuf().writeable(packet->size());
    if (rawBuf.second >= packet->size())
    {
        packet->copy(rawBuf.first, rawBuf.second);
        conn->sendBuf().commitWrite(packet->size());
        if (conn->trySend())
            return true;
    }
    
    auto copySize = packet->copy(rawBuf.first, rawBuf.second);
    conn->sendBuf().commitWrite(copySize);
    packet->pop(copySize);

    connHolder->sendQueue.push_back(packet);
    LOG_TRACE("发送过慢，发送队列出现排队, connId:{}, remote={}", connHolder->id, connHolder->conn->getRemoteEndpoint());
    m_epoller.modifySocket(connHolder->conn->getFD(), net::Epoller::Event::read_write);
    return true;
}

uint32_t TcpConnectionManager::totalPrivateConnNum() const
{
    uint32_t ret = 0;
    for(auto& item : m_privateConns)
        ret += item.second.size();

    return ret;
}

}}
