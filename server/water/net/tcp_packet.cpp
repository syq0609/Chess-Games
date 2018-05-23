#include "tcp_packet.h"
#include <cstring>
#include "../componet/logger.h"
#include "../componet/string_kit.h"

namespace water{
namespace net{

TcpPacket::Ptr TcpPacket::tryParse(const char* data, SizeType size)
{
    auto ret = TcpPacket::create();
    ret->parse(data, size);
    return ret->complete() ? ret : nullptr;
}

TcpPacket::TcpPacket()
    : Packet()
{
}

TcpPacket::TcpPacket(SizeType contentSize)
    : net::Packet(sizeof(SizeType) + contentSize)
{
    std::memcpy(data(), &contentSize, sizeof(contentSize));
}

void TcpPacket::setContent(const void* content, SizeType contentSize)
{
    const SizeType packetSize = sizeof(SizeType) + contentSize;
    m_buf.resize(packetSize);
    std::memcpy(data(), &contentSize, sizeof(contentSize));
    std::memcpy(data() + sizeof(contentSize), content, contentSize);
}

void* TcpPacket::content()
{
    if (size() < sizeof(SizeType))
        return nullptr;

    return data() + sizeof(SizeType);
}

TcpPacket::SizeType TcpPacket::contentSize() const
{
    if (size() < sizeof(SizeType))
        return 0;

    return *reinterpret_cast<const SizeType*>(data());
}

bool TcpPacket::complete() const
{
    return ( size() >= sizeof(SizeType) ) && ( contentSize() + sizeof(SizeType) <= size() );
}

TcpPacket::SizeType TcpPacket::parse(const char* buf, SizeType len)
{
    if (complete()) //已完整, 不再需要解析
        return 0;

    //fill header and try fill body, 包头要一次性读完, 不够则不读
    if (size() == 0)
    {
        if (len < sizeof(SizeType))
            return 0;
        SizeType readLen = *reinterpret_cast<const SizeType*>(buf) + sizeof(SizeType);
        if (readLen > len)
            readLen = len;
        m_buf.assign(buf, buf + readLen);
        return readLen;
    }

    //fill header //处理一下吧, 应该不可能
    SizeType headerAppendLen = 0;
    if (size() < sizeof(SizeType))
    {
        if (len < sizeof(SizeType) - size()) //不够header, 直接不读了
        {
            return 0;
        }
        headerAppendLen = append(buf, sizeof(SizeType) - size());
        len -= headerAppendLen;
        buf += headerAppendLen;
    }
    
    //fill body
    SizeType completeSize = *reinterpret_cast<const SizeType*>(data()) + sizeof(SizeType);
    SizeType deficiencySize = completeSize - size();

    SizeType appendLen = deficiencySize < len ? deficiencySize : len;
    append(buf, appendLen);
    return  headerAppendLen + appendLen;
}


}}
