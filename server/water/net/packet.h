/*
 * Author: LiZhaojia 
 *
 * Last modified: 2014-10-13 21:37 +0800
 *
 * Description:  
 */

#ifndef WATER_NET_PACKET_H
#define WATER_NET_PACKET_H

#include <vector>

#include "../componet/class_helper.h"

#include "net_exception.h"

namespace water{
namespace net{

DEFINE_EXCEPTION(PacketCursorOutOfRange, net::NetException);

class Packet
{
public:
    typedef uint32_t SizeType;
    static const SizeType HEAD_SIZE = sizeof(SizeType);
    static const SizeType MIN_SIZE = sizeof(HEAD_SIZE);
    static const SizeType MAX_SIZE = 65535;
    static const SizeType MIN_CONTENT_SIZE = 0;
    static const SizeType MAX_CONTENT_SIZE = MAX_SIZE - MIN_SIZE;

public:
    TYPEDEF_PTR(Packet);
    CREATE_FUN_MAKE(Packet);

    explicit Packet();
    explicit Packet(SizeType size);
    explicit Packet(const void* data, SizeType size);

    //得到内部buff的地址
    char* data();
    const char* data() const;

    //追加数据
    SizeType append(const void* data, SizeType size);

    //删除数据
    void pop(SizeType size);

    //把数据拷出
    SizeType copy(void* buff, SizeType maxSize);

    //重新设置buff的长度
    //得到buff的长度
    SizeType size() const;

protected:
    std::vector<char> m_buf;
};

}}

#endif

