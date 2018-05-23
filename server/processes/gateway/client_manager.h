/*
 * Author: LiZhaojia - waterlzj@gmail.com
 *
 * Last modified: 2017-05-10 16:31 +0800
 *
 * Description: 客户端管理器
 */

#ifndef PROCESS_GATEWAY_CLIENT_MANAGER_H
#define PROCESS_GATEWAY_CLIENT_MANAGER_H

#include "componet/class_helper.h"
#include "componet/spinlock.h"
#include "componet/event.h"
#include "componet/fast_travel_unordered_map.h"

#include "net/endpoint.h"

#include "base/process_id.h"

#include "protocol/protobuf/proto_manager.h"

#include <list>

namespace water{
namespace net{
}}


namespace gateway{

using namespace water;
using namespace process;


class ClientManager
{

    class Client
    {
    public:
        enum class State
        {
            logining,
            playing,
        };

        TYPEDEF_PTR(Client)
        CREATE_FUN_MAKE(Client)

    public:
        ClientConnectionId ccid = INVALID_CCID;
        ClientUniqueId cuid = INVALID_CUID;
        State state = State::logining;
        net::Endpoint ep;
    };

public:
    TYPEDEF_PTR(ClientManager)
    CREATE_FUN_MAKE(ClientManager)

    NON_COPYABLE(ClientManager)

    ClientManager(ProcessId pid);
    ~ClientManager() = default;

    void timerExec(const componet::TimePoint& now);

    //添加一个client connection, 返回分配给这个conn的clientId，失败返回INVALID_CLIENT_IDENDITY_VALUE
    ClientConnectionId clientOnline(const net::Endpoint& ep);
    void clientOffline(ClientConnectionId ccid);
    void kickOutClient(ClientConnectionId ccid, bool delay = true);
    void sendServerVisionToClient(ClientConnectionId ccid) const;
    void sendSystemNoticeToClient(ClientConnectionId ccid) const;
    void sendSystemNoticeToAllClients() const;

    void regMsgHandler();
    void regClientMsgRelay();

    componet::Event<void (ClientConnectionId)> e_afterEraseClient;

private:
    Client::Ptr createNewClient();
    bool insert(Client::Ptr client);
    void erase(Client::Ptr client);
    Client::Ptr getByCcid(ClientConnectionId ccid);
    void eraseLater(Client::Ptr client);

    void relayClientMsgToServer(const ProcessId& pid, TcpMsgCode code, const ProtoMsgPtr& protoPtr, ClientConnectionId ccid);
    void relayClientMsgToClient(TcpMsgCode code, const ProtoMsgPtr& protoPtr, ClientConnectionId ccid);

    int checkWechatLogin(std::string os, std::string code, std::map<std::string, std::string>& info);

private: //client msg handlers
    void proto_C_Login(ProtoMsgPtr proto, ClientConnectionId ccid);
    void proto_C_LoginOut(ProtoMsgPtr proto, ClientConnectionId ccid);
    void proto_C_Disconnect(ProtoMsgPtr proto, ClientConnectionId ccid);
private: //cluster msg handlers
    void proto_RetLoginQuest(ProtoMsgPtr proto);
    void proto_ClientBeReplaced(ProtoMsgPtr proto);

private:
    const ProcessId m_pid;
    const uint32_t MAX_CLIENT_COUNTER = 0xffffff;
    std::atomic<uint32_t> m_clientCounter;

    componet::Spinlock m_clientsLock; //注意，按照设计，这个锁仅仅保证对m_clients访问的原子性，修改client的数据永远仅在主定时器线程中执行
    componet::FastTravelUnorderedMap<ClientConnectionId, Client::Ptr> m_clients; //只能有这一种key索引，因为重复登陆时，cuid和openid会重复，一并索引很难处理

    componet::Spinlock m_dyingClientsLock;
    std::list<Client::Ptr> m_dyingClients; //即将被销毁的clients
};

} //end namespace gateway


#endif
