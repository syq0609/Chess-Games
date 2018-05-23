/*
 * Author: LiZhaojia 
 *
 * Last modified: 2014-11-27 16:30 +0800
 *
 * Description: 
 */


#ifndef WATER_PROCESS_PRIVATE_CONNECTION_CHECKET_H 
#define WATER_PROCESS_PRIVATE_CONNECTION_CHECKET_H 

#include "componet/event.h"

#include "process_id.h"
#include "process_thread.h"

#include <mutex>
#include <list>
#include <set>
#include <unordered_map>

namespace water{
namespace net {class BufferedConnection;}
namespace process{

class PrivateConnectionChecker : public ProcessThread
{
public:
    TYPEDEF_PTR(PrivateConnectionChecker)
    CREATE_FUN_MAKE(PrivateConnectionChecker)

    PrivateConnectionChecker(ProcessId processId);
    ~PrivateConnectionChecker() = default;

    enum class ConnType {in, out};
    void addUncheckedConnection(std::shared_ptr<net::BufferedConnection> conn, ConnType type);

public:
    componet::Event<void (std::shared_ptr<net::BufferedConnection>, ProcessId processId)> e_connConfirmed;

private:
    void checkConn();
    bool exec() override;

private:
    std::mutex m_mutex;
    using LockGuard = std::lock_guard<std::mutex>;

    const ProcessId m_processId;

    //链接状态, check函数是个状态机
    enum class ConnState 
    {
        recvId,  //in,  等待对方发送身份信息
        sendRet, //in,  发送验证结果给对方
        sendId,  //out, 发送自己的信息给验证方
        recvRet, //out, 接收验证结果
    };
    struct ConnInfo
    {
        ConnState state;
        std::shared_ptr<net::BufferedConnection> conn;
        ProcessId remoteId;
        bool checkResult;
    };

    std::list<ConnInfo> m_conns;

    struct ConnCheckRetMsg
    {
        ProcessId pid;
        bool result;
    };
};

}}

#endif
