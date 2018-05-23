/*
 * Author: LiZhaojia 
 *
 * Last modified: 2014-10-11 23:02 +0800
 *
 * Description: 
 */

#ifndef WATER_HTTP_CONNECTION_MANAGER_H
#define WATER_HTTP_CONNECTION_MANAGER_H

#include "net/buffered_connection.h"
#include "net/epoller.h"

#include "componet/event.h"
#include "componet/class_helper.h"
#include "componet/spinlock.h"
#include "componet/lock_free_circular_queue_ss.h"

#include "process_id.h"
#include "process_thread.h"

#include <unordered_map>
#include <atomic>
#include <list>

namespace water{
namespace net {class BufferedConnection; class HttpPacket; class Packet;}
namespace process{

class HttpConnectionManager : public ProcessThread
{
    using HttpPacketPtr = std::shared_ptr<net::HttpPacket>;
    using HttpPacketCPtr = std::shared_ptr<const net::HttpPacket>;
    using PacketPtr = std::shared_ptr<net::Packet>;
public:

    enum class ConnType
    {
        client = 0,
        server = 1,
    };
    struct ConnectionHolder
    {
        TYPEDEF_PTR(ConnectionHolder);

        HttpConnectionId hcid;
        std::shared_ptr<net::BufferedConnection> conn;
        ConnType type = ConnType::client;
        HttpPacketPtr recvPacket = nullptr;
        //由于socke太忙而暂时无法发出的包，缓存在这里
        componet::Spinlock sendLock;
        std::list<PacketPtr> sendQueue; 
    };
public:
    HttpConnectionManager();
    ~HttpConnectionManager() = default;

    bool exec() override;

    bool addConnection(HttpConnectionId hcid, std::shared_ptr<net::BufferedConnection> conn, ConnType type);
    void eraseConnection(HttpConnectionId hcid);

    bool getPacket(ConnectionHolder::Ptr* conn, HttpPacketPtr* packet);
    bool sendPacket(HttpConnectionId hcid, PacketPtr packet);


public:
    componet::Event<void (HttpConnectionId id)> e_afterEraseConn; 

private:
    void epollerEventHandler(int32_t socketFD, net::Epoller::Event event);

    //从接收队列中取出一个packet, 并得到与其相关的conn
    bool sendPacket(ConnectionHolder::Ptr connHolder, PacketPtr packet);

    void eraseConnection(std::shared_ptr<net::BufferedConnection> conn);

private:

    componet::Spinlock m_lock;

    net::Epoller m_epoller;
    //所有的连接, {HttpConnectionId, conn} {SocketFD, conn}
    std::unordered_map<int32_t, ConnectionHolder::Ptr> m_allConns;
    std::unordered_map<HttpConnectionId, ConnectionHolder::Ptr> m_hcid2Conns;;

    componet::LockFreeCircularQueueSPSC<std::pair<ConnectionHolder::Ptr, HttpPacketPtr>> m_recvQueue;
};

}}

#endif
