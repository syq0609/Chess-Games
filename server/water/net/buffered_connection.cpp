#include "buffered_connection.h"
#include "../componet/string_kit.h"
#include "../componet/logger.h"

namespace water{ 
namespace net{

ConnectionBuffer::ConnectionBuffer(uint32_t sizeLimit/* = 2048*/)
    : m_buf(sizeLimit)//先按定长处理, 带有最大长度限制的变长buf支持, 留待以后再做优化
{
}

std::pair<char*, uint32_t> ConnectionBuffer::readable()
{
    std::pair<char*, uint32_t> ret;
    ret.first  = m_buf.data() + m_dataBegin;
    ret.second = m_dataEnd - m_dataBegin;
    return ret;
}

bool ConnectionBuffer::commitRead(uint32_t size)
{
    if (m_dataEnd - m_dataBegin < size)
        return false;

//    LOG_DEBUG("SOCKT DEBUG, COMMIT_R, this={}, size={}, {}", (uint64_t)this, size, componet::dataToHex(m_buf.data() + m_dataBegin, size));
    m_dataBegin += size;
    if (m_dataBegin == m_dataEnd) //读空了
    {
        m_dataBegin = 0;
        m_dataEnd = 0;
    }

//    LOG_DEBUG("SOCKT DEBUG, COMMIT_R, this={}, aftercomit={}, {}", (uint64_t)this, m_dataEnd - m_dataBegin, componet::dataToHex(m_buf.data() + m_dataBegin, m_dataEnd - m_dataBegin)); 
    return true;
}

std::pair<char*, uint32_t> ConnectionBuffer::writeable(uint32_t size)
{
    std::pair<char*, uint32_t> ret(m_buf.data() + m_dataEnd, 0);
    if (m_buf.size() - m_dataEnd < size || m_buf.size() == m_dataEnd) //尾部空闲空间不足
    {
        if (full()) //确实没空间了
            return ret;
        uint32_t dataSize = m_dataEnd - m_dataBegin;
        memmove(m_buf.data(), m_buf.data() + m_dataBegin, dataSize);
        m_dataBegin = 0;
        m_dataEnd = dataSize;
    }
    ret.first  = m_buf.data() + m_dataEnd;
    ret.second = m_buf.size() - m_dataEnd;
    return ret;
}

bool ConnectionBuffer::commitWrite(uint32_t size)
{
    if (m_buf.size() - m_dataEnd < size) //
        return false;

//    LOG_DEBUG("SOCKT DEBUG, COMMIT_W, this={}, size={}, {}", (uint64_t)this, size, componet::dataToHex(m_buf.data() + m_dataEnd, size));
    m_dataEnd += size;
//    LOG_DEBUG("SOCKT DEBUG, COMMIT_W, this={}, ftercommit={}, {}",(uint64_t)this, m_dataEnd - m_dataBegin, componet::dataToHex(m_buf.data() + m_dataBegin, m_dataEnd - m_dataBegin)); 
    return true;
}

///////////////////////////////////////////////////////////////////////////

BufferedConnection::BufferedConnection(TcpConnection&& tcpConn)
:TcpConnection(std::forward<TcpConnection&&>(tcpConn))
{
}

bool BufferedConnection::trySend()
{
    const auto& rawBuf = m_sendBuf.readable();
    if (rawBuf.second == 0)
        return true;

    const auto sendSize = send(rawBuf.first, rawBuf.second);
    if (sendSize == -1) //socket忙, 数据无法写入
        return false;
    
//    LOG_DEBUG("SOCKT DEBUG, SEND_OK, sbuf={}, zie={}, {}", (uint64_t)&m_sendBuf, sendSize, componet::dataToHex(rawBuf.first, sendSize));
    m_sendBuf.commitRead(sendSize); //不可能失败
    return m_sendBuf.empty();
}

bool BufferedConnection::tryRecv()
{
    const auto& rawBuf = m_recvBuf.writeable();
    if (rawBuf.second == 0)
        return !m_recvBuf.empty();

    const auto recvSize = recv(rawBuf.first, rawBuf.second);
    if (recvSize == 0)
        SYS_EXCEPTION(ReadClosedConnection, "BufferedConnection::tryRecv");

    if(recvSize == -1) //暂时无数据
        return !m_recvBuf.empty();

//    LOG_DEBUG("SOCKT DEBUG, RECV_OK, rbuf={}, szie={}, {}", (uint64_t)&m_recvBuf, recvSize, componet::dataToHex(rawBuf.first, recvSize));
    m_recvBuf.commitWrite(recvSize); //不可能失败
    return true;
}

}}

