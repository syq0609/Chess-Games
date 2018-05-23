/*
 * Author: LiZhaojia
 *
 * Created: 2015-03-06 17:23 +0800
 *
 * Modified: 2015-03-06 17:23 +0800
 *
 * Description:  Tcp消息结构定义
 */


#ifndef WATER_PROCESS_MESSAGE_H
#define WATER_PROCESS_MESSAGE_H

#include <cstdint>
#include <cstring>

#include "process_id.h"

namespace water{
namespace process{


using TcpMsgCode = uint32_t;

/****************消息号的结构*****************/
//最高1bit表示可访问性,  第2bit表示是否为转发, 第3-4bits为协议类型
//把对外的raw和public位都留成0, 这样自动累加消息号时端感觉不出来有协议和可见性区分
//0为特殊消息号, 代表Envelop, 用与转发消息
struct MsgCodeTypeMask
{
    static const TcpMsgCode accessType       = 0xc0000000;  //1-2bit, 可访问性
    static const TcpMsgCode msgPublic        = 0x00000000;  //0b00表示public
    static const TcpMsgCode msgPrivate       = 0x40000000;  //0b01表示private
    static const TcpMsgCode msgEnvelope      = 0xc0000000;  //0b11表示信封消息(11包含01，信封全是private的)

    static const TcpMsgCode protocolType     = 0x30000000; //3-4bits, 协议编码
    static const TcpMsgCode protocolRaw      = 0x10000000; //0b01表示raw
    static const TcpMsgCode protocolProtobuf = 0x00000000; //0b00表示protobuf
};

const TcpMsgCode minPublicRawMsgCode = MsgCodeTypeMask::protocolRaw | MsgCodeTypeMask::msgPublic;
const TcpMsgCode minPrivateRawMsgCode = MsgCodeTypeMask::protocolRaw | MsgCodeTypeMask::msgPrivate;

const TcpMsgCode minPublicProtoMsgCode = MsgCodeTypeMask::protocolProtobuf | MsgCodeTypeMask::msgPublic;
const TcpMsgCode mminPrivateProtoMsgCode = MsgCodeTypeMask::protocolProtobuf | MsgCodeTypeMask::msgPrivate;

/********************消息号的判定*************************/
inline bool isEnvelopeMsgCode(TcpMsgCode code)
{
    return (code & MsgCodeTypeMask::accessType) == MsgCodeTypeMask::msgEnvelope;
}

inline bool isPrivateMsgCode(TcpMsgCode code)
{
    return (code & MsgCodeTypeMask::accessType) == MsgCodeTypeMask::msgPrivate;
}

inline bool isPublicMsgCode(TcpMsgCode code)
{
    return (code & MsgCodeTypeMask::accessType) == MsgCodeTypeMask::msgPublic;
}

inline bool isRawMsgCode(TcpMsgCode code)
{
    return (code & MsgCodeTypeMask::protocolType) == MsgCodeTypeMask::protocolRaw;
}

inline bool isProtobufMsgCode(TcpMsgCode code)
{
    return (code & MsgCodeTypeMask::protocolType) == MsgCodeTypeMask::protocolProtobuf;
}


/*********************消息结构的定义*********************/
#pragma pack(1)


struct TcpMsg
{
    TcpMsg() = default;
    TcpMsg(TcpMsgCode code_) : code(code_) {}

    TcpMsgCode code;       //由消息号生成机制保证privateCode <= 1000;
    uint8_t    data[0];     //实际的消息
};
struct Envelope : public TcpMsg
{
    Envelope(TcpMsgCode code)
    : TcpMsg(MsgCodeTypeMask::msgEnvelope | 
             (code & MsgCodeTypeMask::protocolType)) //信封的协议类型, 同其携带的消息
      , targetPid(0)
      , msg(code)
    {
    }

    void fill(const void* buf, uint32_t size)
    {
        if(buf != nullptr && size > 0)
            std::memcpy(data, buf, size);
    }

    ProcessIdValue    targetPid;   //最终目标进程Id
    uint64_t   sourceId;                 //实际的msg来源Id
    TcpMsg     msg;
};
#pragma pack()

}}


#endif

