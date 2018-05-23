/*
 * Author: LiZhaojia 
 *
 * Last modified: 2014-09-29 19:06 +0800
 *
 * Description: 
 */

#include "connection.h"

#include "net_exception.h"

#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h> //htons

namespace water{
namespace net{

TcpConnection::Ptr TcpConnection::connect(const std::string& endpointStr)
{
    Endpoint ep(endpointStr);
    return connect(ep);
}

TcpConnection::Ptr TcpConnection::connect(const std::string& strIp, uint16_t port)
{
    Endpoint ep;
    ep.ip.fromString(strIp);
    ep.port = port;
    return connect(ep);
}

TcpConnection::Ptr TcpConnection::connect(const Endpoint& remoteEndpoint)
{
    ::sockaddr_in serverAddrIn;
    std::memset(&serverAddrIn, 0, sizeof(serverAddrIn));

    serverAddrIn.sin_family      = AF_INET;
    serverAddrIn.sin_addr.s_addr = remoteEndpoint.ip.value;
    serverAddrIn.sin_port        = htons(remoteEndpoint.port);

    TcpConnection::Ptr ret = TcpConnection::create(remoteEndpoint);
    if(-1 == ::connect(ret->getFD(), (const struct sockaddr*)&serverAddrIn, sizeof(serverAddrIn)))
        SYS_EXCEPTION(NetException, "::connect:");

    return ret;
}

bool TcpConnection::tryConnect()
{
    ::sockaddr_in serverAddrIn;
    std::memset(&serverAddrIn, 0, sizeof(serverAddrIn));

    serverAddrIn.sin_family      = AF_INET;
    serverAddrIn.sin_addr.s_addr = m_remoteEndpoint.ip.value;
    serverAddrIn.sin_port        = htons(m_remoteEndpoint.port);

    setNonBlocking();

    const int32_t connectRet = ::connect(getFD(), (const ::sockaddr*)&serverAddrIn, sizeof(serverAddrIn));

    //直接成功了
    if(connectRet == 0)
        return true;

    if(errno == EISCONN)
        return true;

    if(errno != EINPROGRESS && errno != EALREADY)
        SYS_EXCEPTION(NetException, "tryConnect, ::connect:");

    return false;
}

TcpConnection::Ptr TcpConnection::connect(const std::string& endpointStr, const std::chrono::milliseconds& timeout)
{
    Endpoint ep(endpointStr);
    return connect(ep, timeout);
}

TcpConnection::Ptr TcpConnection::connect(const std::string& strIp, uint16_t port, const std::chrono::milliseconds& timeout)
{
    Endpoint ep;
    ep.ip.fromString(strIp);
    ep.port = port;
    return connect(ep, timeout);
}

TcpConnection::Ptr TcpConnection::connect(const Endpoint& remoteEndpoint, const std::chrono::milliseconds& timeout)
{
    ::sockaddr_in serverAddrIn;
    std::memset(&serverAddrIn, 0, sizeof(serverAddrIn));

    serverAddrIn.sin_family      = AF_INET;
    serverAddrIn.sin_addr.s_addr = remoteEndpoint.ip.value;
    serverAddrIn.sin_port        = htons(remoteEndpoint.port);

    TcpConnection::Ptr ret = TcpConnection::create(remoteEndpoint);

    //用非阻塞方式来连接
    ret->setNonBlocking();
    const int32_t connectRet = ::connect(ret->getFD(), (const ::sockaddr*)&serverAddrIn, sizeof(serverAddrIn));

    //直接成功了
    if(connectRet == 0)
    {
        ret->setBlocking();
        return ret;
    }

    //确认是否正在连接中
    if(errno != EINPROGRESS)
        SYS_EXCEPTION(NetException, "::connect:");

    //用select实现超时等待
    const int32_t fd = ret->getFD();
    ::timeval t;
    t.tv_sec = timeout.count() / 1000;
    t.tv_usec = timeout.count() % 1000;

    fd_set writeSet, errSet;
    FD_ZERO(&writeSet);
    FD_ZERO(&errSet);
    FD_SET(fd, &writeSet);
    FD_SET(fd, &errSet);

    const int selectRet = ::select(fd + 1, nullptr, &writeSet, &errSet, &t);
    if(selectRet > 0) //正常返回，检查返回原因，确定是否连接成功
    {
        int32_t connectErrno = 0;
        socklen_t connectErrnoLen = sizeof(connectErrnoLen);
        if(-1 == ::getsockopt(fd, SOL_SOCKET, SO_ERROR, &connectErrno, &connectErrnoLen))
            SYS_EXCEPTION(NetException, "::getsocketopt");

        if(connectErrno != 0)//连接失败
        {
            errno = connectErrno;
            SYS_EXCEPTION(NetException, "::connect");
        }
    }
    else if(selectRet == 0) //超时返回
    {
        return nullptr;
    }
    else //select 出错
    {
        SYS_EXCEPTION(NetException, "::select");
    }

    //select非超时返回，检查socket是否出错

    ret->setBlocking();
    return ret;
}
///////////////////////////////////////////////////////////////////////
TcpConnection::TcpConnection(const Endpoint& remoteEndpoint)
: m_remoteEndpoint(remoteEndpoint), m_state(ConnState::read_nad_write)
{
}

TcpConnection::TcpConnection(int32_t socketFD, const Endpoint& remoteEndpoint)
: TcpSocket(socketFD), m_remoteEndpoint(remoteEndpoint), m_state(ConnState::read_nad_write)
{
}

TcpConnection::~TcpConnection()
{
    try
    {
        if(getState() != ConnState::closed)
            close();
    }
    catch(...)
    {
    }
}

TcpConnection::TcpConnection(TcpConnection&& other)
    :TcpSocket(std::move(other)),
    m_remoteEndpoint(std::move(other.m_remoteEndpoint)), 
    m_state(std::move(other.m_state))
{
}

TcpConnection& TcpConnection::operator=(TcpConnection&& other)
{
    TcpSocket::operator=(std::move(other));

    m_remoteEndpoint = std::move(m_remoteEndpoint);
    m_state = std::move(other.m_state);
    return *this;
}

const Endpoint& TcpConnection::getRemoteEndpoint() const
{
    return m_remoteEndpoint;
}

void TcpConnection::shutdown(ConnState state)
{
    if(!isAvaliable())
        return;

    switch (state)
    {
    case ConnState::read:
        ::shutdown(getFD(), SHUT_RD);
    case ConnState::write:
        ::shutdown(getFD(), SHUT_WR);
        break;
    case ConnState::read_nad_write:
        ::shutdown(getFD(), SHUT_RDWR);
        break;
    default:
        break;
    }

    const uint8_t stateValue = (uint8_t)state;
    const uint8_t m_stateValue = (uint8_t)m_state;
    m_state = (ConnState)(m_stateValue & ~stateValue);

    if(m_state == ConnState::closed)
        close();
}

TcpConnection::ConnState TcpConnection::getState() const
{
    return m_state;
}

int32_t TcpConnection::send(const void* data, int32_t dataLen)
{
    uint32_t sendLen = ::send(getFD(), data, dataLen, MSG_NOSIGNAL);
    if(sendLen == static_cast<uint32_t>(-1))
    {
        if(errno == EAGAIN || errno == EWOULDBLOCK)
            return -1;

        SYS_EXCEPTION(NetException, "::send");
    }

    return sendLen;
}

int32_t TcpConnection::recv(void* data, int32_t dataLen)
{
    uint32_t recvLen = ::recv(getFD(), data, dataLen, 0);
    if(recvLen == static_cast<uint32_t>(-1))
    {
        if(errno == EAGAIN || errno == EWOULDBLOCK)
            return -1;
        SYS_EXCEPTION(NetException, "::recv");
    }
    return recvLen;
}


}}

