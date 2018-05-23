#include "lobby.h"

#include "client_manager.h"
#include "water13.h"
#include "game_config.h"

#include "componet/logger.h"
#include "componet/scope_guard.h"
#include "net/endpoint.h"
#include "dbadaptcher/redis_handler.h"

#include "base/tcp_message.h"

#include "protocol/rawmsg/rawmsg_manager.h"
#include "protocol/protobuf/proto_manager.h"


//#include "dbadaptcher/dbconnection_pool.h"

namespace lobby{

using protocol::rawmsg::RawmsgManager;


Lobby* Lobby::m_me = nullptr;

Lobby& Lobby::me()
{
    return *m_me;
}

void Lobby::init(int32_t num, const std::string& configDir, const std::string& logDir)
{
    m_me = new Lobby(num, configDir, logDir);
}

Lobby::Lobby(int32_t num, const std::string& configDir, const std::string& logDir)
: Process("lobby", num, configDir, logDir)
{
}

void Lobby::init()
{
    //先执行基类初始化
    process::Process::init();

    //加载配置
    ProtoManager::me().loadConfig(m_cfgDir);
    GameConfig::me().load(m_cfgDir);
    dbadaptcher::RedisHandler::me().setCfgDir(m_cfgDir);
    //初始化初始化ClientManager
    ClientManager::me().init();
    //加载G13游戏房间数据
    Water13::recoveryFromDB();
    //注册消息处理事件和主定时器事件
    registerTcpMsgHandler();
    registerTimerHandler();
}

void Lobby::lanchThreads()
{
    Process::lanchThreads();
}

void Lobby::stop()
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

bool Lobby::sendToPrivate(ProcessId pid, TcpMsgCode code)
{
    return sendToPrivate(pid, code, nullptr, 0);
}

bool Lobby::sendToPrivate(ProcessId pid, TcpMsgCode code, const ProtoMsg& proto)
{
    return relayToPrivate(getId().value(), pid, code, proto);
}

bool Lobby::relayToPrivate(uint64_t sourceId, ProcessId pid, TcpMsgCode code, const ProtoMsg& proto)
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

bool Lobby::sendToPrivate(ProcessId pid, TcpMsgCode code, const void* raw, uint32_t size)
{
    return relayToPrivate(getId().value(), pid, code, raw, size);
}

bool Lobby::relayToPrivate(uint64_t sourceId, ProcessId pid, TcpMsgCode code, const void* raw, uint32_t size)
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


void Lobby::tcpPacketHandle(TcpPacket::Ptr packet, 
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

