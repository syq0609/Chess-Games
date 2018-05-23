/*
 * Author: LiZhaojia 
 *
 * Last modified: 2014-12-03 10:46 +0800
 *
 * Description: 
 */

#ifndef PROCESS_LOBBY_LOBBY_H
#define PROCESS_LOBBY_LOBBY_H


#include "base/process.h"
#include "protocol/rawmsg/commdef.h"
#include "protocol/protobuf/proto_manager.h"

namespace lobby{

using namespace water;
using namespace process;
//using protocol::protobuf::ProtoMsg;
//using protocol::protobuf::ProtoMsgPtr;
//using protocol::protobuf::ProtoManager;


class Lobby : public process::Process
{
public:
    bool sendToPrivate(ProcessId pid, TcpMsgCode code);
    bool sendToPrivate(ProcessId pid, TcpMsgCode code, const ProtoMsg& proto);
    bool sendToPrivate(ProcessId pid, TcpMsgCode code, const void* raw, uint32_t size);

    bool relayToPrivate(uint64_t sourceId, ProcessId pid, TcpMsgCode code, const ProtoMsg& proto);
    bool relayToPrivate(uint64_t sourceId, ProcessId pid, TcpMsgCode code, const void* raw, uint32_t size);

private:
    Lobby(int32_t num, const std::string& configDir, const std::string& logDir);

    void tcpPacketHandle(TcpPacket::Ptr packet, 
                         TcpConnectionManager::ConnectionHolder::Ptr conn,
                         const componet::TimePoint& now) override;

    void init() override;
    void lanchThreads() override;
	void stop() override;

    void registerTcpMsgHandler();
    void registerTimerHandler();

    //void loadConfig();

public:
    static void init(int32_t num, const std::string& configDir, const std::string& logDir);
    static Lobby& me();
private:
    static Lobby* m_me;
};

}

#endif
