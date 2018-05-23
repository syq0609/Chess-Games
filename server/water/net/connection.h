/*
 * Author: LiZhaojia 
 *
 * Last modified: 2014-09-28 19:04 +0800
 *
 * Description: 
 */

#ifndef WATER_NET_CONNECTION_HPP
#define WATER_NET_CONNECTION_HPP

#include "socket.h"
#include "net_exception.h"
#include "endpoint.h"
//#include "componet/lock_free_circular_queue_ss.h"

#include <chrono>

namespace water{ 
namespace net{

class TcpConnection : public TcpSocket
{
public:
    TYPEDEF_PTR(TcpConnection)
    CREATE_FUN_NEW(TcpConnection)

private:
    explicit TcpConnection(const Endpoint& remoteEndpoint);
    explicit TcpConnection(int32_t socketFD, const Endpoint& remoteEndpoint);

public:
    ~TcpConnection();

    //move able
    TcpConnection(TcpConnection&& other);
    TcpConnection& operator=(TcpConnection&& other);

public:
    enum class ConnState : uint8_t 
    {
        closed         = 0, 
        read           = 1, 
        write          = 2, 
        read_nad_write = 3
    };

    const Endpoint& getRemoteEndpoint() const;

    void shutdown(ConnState state = ConnState::read_nad_write);

    ConnState getState() const;

    //非阻塞的方式发起连接, 成功返回true, 暂时无法成功返回false, 出错抛出异常
    //调用此函数后, 本对象关联的socketFD会被设置为非阻塞
    bool tryConnect();

public://发送与接收
    //返回-1时, 表示noblocking的socket, 返回EAGAIN或EWOULDBLOCK, 即socket忙
    int32_t send(const void* data, int32_t dataLen);
    //返回-1时, 表示noblocking的socket, 返回EAGAIN或EWOULDBLOCK, 即socket忙
    //返回0时, 表示socket不可写, 即发送已被关闭
    int32_t recv(void* data, int32_t dataLen);

private:
    Endpoint m_remoteEndpoint;
    ConnState m_state;

public:
    static TcpConnection::Ptr connect(const std::string& endpointStr); //xxx.xxx.xxx.xxx:prot
    static TcpConnection::Ptr connect(const std::string& strIp, uint16_t port);
    static TcpConnection::Ptr connect(const Endpoint& endPoint);
    static TcpConnection::Ptr connect(const std::string& endpointStr, const std::chrono::milliseconds& timeout);
    static TcpConnection::Ptr connect(const std::string& strIp, uint16_t port, const std::chrono::milliseconds& timeout);
    static TcpConnection::Ptr connect(const Endpoint& endPoint, const std::chrono::milliseconds& timeout);
};

}}

#endif

