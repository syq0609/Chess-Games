/*
 * Author: LiZhaojia 
 *
 * Last modified: 2014-12-03 10:46 +0800
 *
 * Description:  game server 的进程定义
 */

#ifndef PROCESS_GATEWAY_GATEWAY_H
#define PROCESS_GATEWAY_GATEWAY_H


#include "protocol/protobuf/proto_manager.h"
#include "base/process.h"
//#include "base/def.h"

#include "client_manager.h"


namespace gateway{


using namespace water;
using namespace process;

class Gateway : public process::Process
{
public:
    bool sendToPrivate(ProcessId pid, TcpMsgCode code);
    bool sendToPrivate(ProcessId pid, TcpMsgCode code, const ProtoMsg& proto);
    bool sendToPrivate(ProcessId pid, TcpMsgCode code, const void* raw, uint32_t size);

    bool relayToPrivate(uint64_t sourceId, ProcessId pid, TcpMsgCode code, const ProtoMsg& proto);
    bool relayToPrivate(uint64_t sourceId, ProcessId pid, TcpMsgCode code, const void* raw, uint32_t size);

    bool sendToClient(ClientConnectionId ccid, TcpMsgCode code, const void* raw, uint32_t size);
    bool sendToClient(ClientConnectionId ccid, TcpMsgCode code, const ProtoMsg& proto);

    net::BufferedConnection::Ptr eraseClientConn(ClientConnectionId ccid);

    HttpConnectionManager& httpConnectionManager();

    ClientManager::Ptr clientManager() const;

private:
    Gateway(int32_t num, const std::string& configDir, const std::string& logDir);

    void tcpPacketHandle(TcpPacket::Ptr packet, 
                         TcpConnectionManager::ConnectionHolder::Ptr conn,
                         const componet::TimePoint& now) override;

    void init() override;
    void lanchThreads() override;

    void loadConfig();

    //处理新接入的客户端连接
    void newClientConnection(net::BufferedConnection::Ptr conn);

    //连接到平台成功
    void newPlatformConnection(net::BufferedConnection::Ptr conn);

    void registerTcpMsgHandler();
    void registerTimerHandler();

private:
    ClientManager::Ptr m_clientManager;
    TcpClient::Ptr m_platformClient;

public:
    static void init(int32_t num, const std::string& configDir, const std::string& logDir);
    static Gateway& me();
private:
    static Gateway* m_me;
};

}

#endif
