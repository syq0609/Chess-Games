/*
 * Author: LiZhaojia - dantezhu@vip.qq.com
 *
 * Last modified: 2017-06-17 02:52 +0800
 *
 * Description: 
 */

#ifndef WATER_TCP_CLIENT_H
#define WATER_TCP_CLIENT_H

#include <atomic>
#include <map>

#include "net/endpoint.h"
#include "componet/datetime.h"
#include "componet/event.h"
#include "componet/class_helper.h"
#include "process_thread.h"

namespace water{

namespace net {class BufferedConnection; class TcpConnection;}

namespace process{


class TcpClient : public ProcessThread
{
public:
    TYPEDEF_PTR(TcpClient)
    CREATE_FUN_MAKE(TcpClient)

    TcpClient();
    void addRemoteEndpoint(net::Endpoint ep, std::chrono::seconds retryInterval);
    bool exec() override;

public:
    componet::Event<void (std::shared_ptr<net::BufferedConnection>)> e_newConn;
    componet::Event<void (TcpClient*)> e_close;

private:
    uint32_t readyNum() const;

private:
    struct RemoteEndpointInfo
    {
        componet::TimePoint retryTimepoint;
        std::chrono::seconds retryInterval;
        std::weak_ptr<net::TcpConnection> conn;
    };

    std::map<net::Endpoint, RemoteEndpointInfo> m_remoteEndpoints;
};

}}

#endif
