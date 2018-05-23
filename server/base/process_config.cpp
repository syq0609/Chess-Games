#include "process_config.h"

#include "componet/logger.h"
#include "componet/xmlparse.h"
#include "componet/format.h"
#include "componet/string_kit.h"

namespace water{
namespace process{


ProcessConfig::ProcessConfig(const std::string& processName, int16_t processNum)
:m_processName(processName), m_processNum(processNum)
{
}

void ProcessConfig::load(const std::string& cfgDir)
{
    using componet::XmlParseDoc;
    using componet::XmlParseNode;

    const std::string configFile = cfgDir + "/process.xml";

    LOG_TRACE("读取集群拓扑配置 {}", configFile);

    XmlParseDoc doc(configFile);
    XmlParseNode root = doc.getRoot();
    if(!root)
        EXCEPTION(LoadProcessConfigFailed, configFile + " parse root node failed");

    {//读取redis配置
        XmlParseNode redisNode = root.getChild("redis");
        if (!redisNode)
            LOG_TRACE("redis cfg dose not exisit, use default values");
        m_redis.host = redisNode.getAttr<std::string>("host");
        m_redis.port = redisNode.getAttr<int32_t>("port");
    }
    {//进程拓扑结构
        XmlParseNode allProcessesNode = root.getChild("allProcesses");
        if(!allProcessesNode)
            EXCEPTION(LoadProcessConfigFailed, "allProcesses node not exisit");

        {//进程类型声明, 生成并建立processType和processName的映射, 记录在ProcessIdendity中

            const std::string allTypeStr = allProcessesNode.getAttr<std::string>("allType");
            std::vector<std::string> allTypeNames = componet::splitString(allTypeStr, " ");

            for(auto& name : allTypeNames)
            {
                //ProcessIdendity::type2Name
                ProcessId::s_type2Name.push_back(name);

                //ProcessIdendity::name2Type
                ProcessType type = ProcessId::s_type2Name.size() - 1;
                ProcessId::s_name2Type[name] = type;
            }
            ProcessId::s_type2Name.shrink_to_fit();
        }

        
        {   //对每个zone节点下的processType节点做一次遍历，
            //做好processId 到privateListen的Endpoint映射
            //并找到当前进程类型对应的配置结点
            XmlParseNode thisProcessId = XmlParseNode::getInvalidNode();
            for(XmlParseNode processTypeNode = allProcessesNode.getChild("processType"); processTypeNode; ++processTypeNode)
            {
                auto name = processTypeNode.getAttr<std::string>("name");
                ProcessType type = ProcessId::stringToType(name);
                if(type == INVALID_PROCESS_TYPE)
                    EXCEPTION(ProcessCfgNotExisit, "{} is not exisit in allProcesses.allType", name);

                //processType.process.private.listen
                for(XmlParseNode processNode = processTypeNode.getChild("process"); processNode; ++processNode)
                {
                    auto num = processNode.getAttr<int32_t>("num");
                    ProcessId processId(name, num);
                    XmlParseNode privateNode = processNode.getChild("private");
                    if(!privateNode)
                        EXCEPTION(ProcessCfgNotExisit, "process cfg {} do not has {} node", processId, "private");

                    auto endPointStr = privateNode.getAttr<std::string>("listen");
                    std::shared_ptr<net::Endpoint> privateListen;
                    if(!endPointStr.empty())
                    {
                        privateListen.reset(new net::Endpoint(endPointStr));
                        m_processIdPrivateListenEps[processId] = *privateListen;
                    }

                    //process self node
                    if(name == m_processName && num == m_processNum)
                    {
                        m_processId.type(type);
                        m_processId.num(m_processNum);

                        thisProcessId = processNode;
                        m_processInfo.privateNet.listen = privateListen;
                    }
                }
            }

            if(!thisProcessId)
            {
                //出错，这此时的m_processId不可靠，日志中的processFullName要自己拼
                EXCEPTION(ProcessCfgNotExisit, "进程[{}-{}]在配置文件中不存在", m_processName, m_processNum);
            }

            ProcessInfo& info = m_processInfo;

            //开始开始解析私网除listen外的部分
            XmlParseNode privateNode = thisProcessId.getChild("private");
            if(!privateNode) //私网配置节点必须存在
                EXCEPTION(ProcessCfgNotExisit, "进程 {} 下缺少 private 配置", m_processId);
            //私网接出
            if(!parsePrivateEndpoint(&info.privateNet.connect, privateNode.getAttr<std::string>("connectTo")))
                EXCEPTION(LoadProcessConfigFailed, "进程 {} 属性 connectTo 解析失败", m_processId);

            //解析公网
            XmlParseNode publicNode = thisProcessId.getChild("public");
            if(publicNode)
                parseEndpointList(&info.publicNet.listen, publicNode.getAttr<std::string>("listen"));

            //解析Flash Sandbox
            XmlParseNode flashSandboxNode = thisProcessId.getChild("flashSandbox");
            if(flashSandboxNode)
                parseEndpointList(&info.flashSandbox.listen, flashSandboxNode.getAttr<std::string>("listen")); 

            //解析http
            XmlParseNode httpNode = thisProcessId.getChild("http");
            if (httpNode)
                parseEndpointList(&info.httpNet.listen, httpNode.getAttr<std::string>("listen"));
        }
    }
}

bool ProcessConfig::parsePrivateEndpoint(std::set<net::Endpoint>* ret, const std::string& str)
{
    ret->clear();

    std::set<ProcessId> processIds;
    if(!parseProcessList(&processIds, str))
        return false;

    for(const auto& processId : processIds)
    {
        auto it = m_processIdPrivateListenEps.find(processId);
        if(it == m_processIdPrivateListenEps.end())
            return false;

        ret->insert(it->second);
    }

    return true;
}

bool ProcessConfig::parseProcessList(std::set<ProcessId>* ret, const std::string& str)
{
    ret->clear();
    std::vector<std::string> items;
    componet::splitString(&items, str, " ");

    for(const std::string& item : items)
    {
        ProcessId id;
        std::vector<std::string> processStr = componet::splitString(item, ":");
        if(processStr.size() ==  2)
        {
            id.type(ProcessId::stringToType(processStr[0]));
            if(id.type() == INVALID_PROCESS_TYPE)
                return false;

            auto processNums = componet::fromString<std::vector<int16_t>>(processStr[1], ",");

            for(int32_t num : processNums)
            {
                id.num(num);
                ret->insert(id);
            }
        }
        else if(processStr.size() == 3)
        {
            id.type(ProcessId::stringToType(processStr[1]) );
            if(id.type() == INVALID_PROCESS_TYPE)
                continue;
            auto processNums = componet::fromString<std::vector<int16_t>>(processStr[2], ",");

            for(int32_t num : processNums)
            {
                id.num(num);
                ret->insert(id);
            }
        }
        else
        {
            return false;
        }
    }

    return true;
}

void ProcessConfig::parseEndpointList(std::set<net::Endpoint>* ret, const std::string& str)
{
    ret->clear();
    std::vector<std::string> endpointStrs = componet::splitString(str, ",");
    for(const auto& endpointStr : endpointStrs)
    {
        net::Endpoint ep(endpointStr);
        ret->insert(ep);
    }
}

const ProcessConfig::ProcessInfo& ProcessConfig::getInfo()
{
    return m_processInfo;
}

ProcessId ProcessConfig::getProcessId() const
{
    return m_processId;
}

componet::TimePoint ProcessConfig::opentime() const
{
    return m_opentime;
}


}}

