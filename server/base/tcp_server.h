/*
 * Author: LiZhaojia 
 *
 * Last modified: 2014-11-24 16:44 +0800
 *
 * Description: 
 */

#ifndef WATER_TCP_SERVER_H
#define WATER_TCP_SERVER_H

#include <set>
#include <unordered_map>

#include "process_thread.h"
#include "net/epoller.h"
#include "net/endpoint.h"
#include "net/listener.h"
#include "componet/event.h"

namespace water{
namespace net {class BufferedConnection; class TcpConnection;}
namespace process{

class TcpServer final : public ProcessThread
{
public:
    TYPEDEF_PTR(TcpServer)
    CREATE_FUN_MAKE(TcpServer)

    TcpServer();
    ~TcpServer() = default;
    void addLocalEndpoint(const net::Endpoint& ep);
    bool exec() override;

public:
    componet::Event<void (std::shared_ptr<net::BufferedConnection>)> e_newConn;

private:
    void epollEventHandler(int32_t socketFD, net::Epoller::Event event);

private:
    std::set<net::Endpoint> m_localEndpoints;
    std::unordered_map<int32_t, net::TcpListener::Ptr> m_listeners; //{socketFD, listener}
    net::Epoller m_epoller;
};

}}

#endif
