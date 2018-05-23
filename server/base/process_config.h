/*
 * Author: LiZhaojia 
 *
 * Last modified: 2014-11-18 12:10 +0800
 *
 * Description:  进程配置，监听的端点，要连接的端点，可接入的processType
 */

#ifndef WATER_PROCESS_NET_CONFIG_H
#define WATER_PROCESS_NET_CONFIG_H

#include <string>
#include <vector>
#include <set>
#include <map>

#include "componet/exception.h"
#include "componet/datetime.h"
#include "net/endpoint.h"
#include "process_id.h"


namespace water{
namespace process{

DEFINE_EXCEPTION(LoadProcessConfigFailed, componet::ExceptionBase)
DEFINE_EXCEPTION(ProcessCfgNotExisit, componet::ExceptionBase)

class ProcessConfig final
{
public:

    struct ProcessInfo
    {
        struct
        {
            std::shared_ptr<net::Endpoint> listen;

            std::set<net::Endpoint> connect;
        } privateNet;

        struct
        {
            std::set<net::Endpoint> listen;
        } publicNet;

        struct
        {
            std::set<net::Endpoint> listen;
        } httpNet;

        struct
        {
            std::set<net::Endpoint> listen;
        } flashSandbox;
    };

    ProcessConfig(const std::string& processName, int16_t processNum);
    ~ProcessConfig() = default;

    void load(const std::string& cfgDir);

    const ProcessInfo& getInfo();
    ProcessId getProcessId() const;

    componet::TimePoint opentime() const;

private:
    bool parsePrivateEndpoint(std::set<net::Endpoint>* ret, const std::string& str);
    bool parseProcessList(std::set<ProcessId>* ret, const std::string& str);
    void parseEndpointList(std::set<net::Endpoint>* ret, const std::string& str);

private:
    componet::TimePoint m_opentime;

    const std::string m_processName;
    const uint16_t m_processNum;
    ProcessId m_processId;

    std::map<ProcessId, net::Endpoint> m_processIdPrivateListenEps;
    ProcessInfo m_processInfo; //所有进程的信息

    struct
    {
        std::string host = "127.0.0.1";
        int32_t port = 6379;
    } m_redis;
};

}}

#endif
