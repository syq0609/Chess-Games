#include "hall.h"

#include "water/componet/logger.h"
#include "water/componet/scope_guard.h"
#include "water/net/endpoint.h"
#include "base/tcp_message.h"
#include "protocol/rawmsg/rawmsg_manager.h"

namespace hall{

using protocol::rawmsg::RawmsgManager;


Hall* Hall::m_me = nullptr;

Hall& Hall::me()
{
    return *m_me;
}

void Hall::init(int32_t num, const std::string& configDir, const std::string& logDir)
{
    m_me = new Hall(num, configDir, logDir);
}

Hall::Hall(int32_t num, const std::string& configDir, const std::string& logDir)
: Process("hall", num, configDir, logDir)
{
}

void Hall::init()
{
    //先执行基类初始化
    process::Process::init();

    //加载配置
    loadConfig();

    //注册消息处理事件和主定时器事件
    registerTcpMsgHandler();
    registerTimerHandler();
}

void Hall::lanchThreads()
{
    Process::lanchThreads();
}

void Hall::stop()
{
    /*
	for(Role::Ptr role : RoleManager::me())
	{
		if(role == nullptr)
			continue;

		role->beforeLeaveScene();
	}
*/
	Process::stop();
}

bool Hall::sendToPrivate(ProcessId pid, TcpMsgCode code)
{
    return sendToPrivate(pid, code, nullptr, 0);
}

bool Hall::sendToPrivate(ProcessId pid, TcpMsgCode code, const ProtoMsg& proto)
{
    return relayToPrivate(getId().value(), pid, code, proto);
}

bool Hall::relayToPrivate(uint64_t sourceId, ProcessId pid, TcpMsgCode code, const ProtoMsg& proto)
{
    const uint32_t protoBinSize = proto.ByteSize();
    const uint32_t bufSize = sizeof(Envelope) + protoBinSize;
    uint8_t* buf = new uint8_t[bufSize];
    ON_EXIT_SCOPE_DO(delete[] buf);

    Envelope* envelope = new(buf) Envelope(code);
    envelope->targetPid  = pid.value();
    envelope->sourceId = sourceId;

    if(!proto.SerializeToArray(envelope->msg.data, protoBinSize))
    {
        LOG_ERROR("proto serialize failed, msgCode = {}", code);
        return false;
    }

    TcpPacket::Ptr packet = TcpPacket::create();
    packet->setContent(buf, bufSize);

    const ProcessId routerId("router", 1);
    return m_conns.sendPacketToPrivate(routerId, packet);
}

bool Hall::sendToPrivate(ProcessId pid, TcpMsgCode code, const void* raw, uint32_t size)
{
    return relayToPrivate(getId().value(), pid, code, raw, size);
}

bool Hall::relayToPrivate(uint64_t sourceId, ProcessId pid, TcpMsgCode code, const void* raw, uint32_t size)
{
    const uint32_t bufSize = sizeof(Envelope) + size;
    uint8_t* buf = new uint8_t[bufSize];
    ON_EXIT_SCOPE_DO(delete[] buf);

    Envelope* envelope = new(buf) Envelope(code);
    envelope->targetPid  = pid.value();
    envelope->sourceId = sourceId;
    std::memcpy(envelope->msg.data, raw, size);

    TcpPacket::Ptr packet = TcpPacket::create();
    packet->setContent(buf, bufSize);

//    LOG_DEBUG("sendToPrivate, rawSize={}, tcpMsgSize={}, packetSize={}, contentSize={}", 
//              size, bufSize, packet->size(), *(uint32_t*)(packet->data()));

    const ProcessId routerId("router", 1);
    return m_conns.sendPacketToPrivate(routerId, packet);
}


void Hall::tcpPacketHandle(TcpPacket::Ptr packet, 
                               TcpConnectionManager::ConnectionHolder::Ptr conn,
                               const componet::TimePoint& now)
{
    if(packet == nullptr)
        return;


    auto tcpMsg = reinterpret_cast<water::process::TcpMsg*>(packet->content());
    if(water::process::isRawMsgCode(tcpMsg->code))
        RawmsgManager::me().dealTcpMsg(tcpMsg, packet->contentSize(), conn->id, now);
    else if(water::process::isProtobufMsgCode(tcpMsg->code))
        ProtoManager::me().dealTcpMsg(tcpMsg, packet->contentSize(), conn->id, now);
}

}

