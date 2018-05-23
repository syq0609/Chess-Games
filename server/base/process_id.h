/*
 * Author: LiZhaojia 
 *
 * Last modified: 2014-11-27 15:45 +0800
 *
 * Description: 进程标识
 */


#ifndef WATER_PROCESS_ID_H
#define WATER_PROCESS_ID_H


#include <string>
#include <vector>
#include <map>


namespace water{
namespace process{

//http 连接ID
typedef uint64_t HttpConnectionId;
const HttpConnectionId INVALID_HCID = 0;


//客户连接ID
typedef uint64_t ClientConnectionId; // |-32bits unixtime-|-8bits gatewayNum-|-24bits counter-|
const ClientConnectionId INVALID_CCID = 0;
//客户唯一ID
typedef uint64_t ClientUniqueId; // |-32bits unixtime-|-32bits counter-|
const ClientUniqueId INVALID_CUID = 0;

// ProcessType, 实际仅使用低8bits
typedef uint32_t ProcessType;
const ProcessType INVALID_PROCESS_TYPE = 0;

// ProcessNum，实际仅使用低8bits
typedef uint32_t ProcessNum;
// ProcessIdValue =  |- 16bits reserve -|- 8bits ProcessType -|- 8bits ProcessNum -|
typedef uint32_t ProcessIdValue;
const ProcessIdValue INVALID_PROCESS_IDENDITY_VALUE = 0;

//服务器ID
class ProcessId
{
public:
    ProcessId(const std::string& typeStr, int8_t num);
    ProcessId(ProcessIdValue value_ = INVALID_PROCESS_IDENDITY_VALUE);

    //copy & assign
    ProcessId(const ProcessId& other) = default;
    ProcessId& operator=(const ProcessId& other) = default;

    void clear();
    bool isValid() const;

    std::string toString() const;

    void setValue(ProcessIdValue value);
    ProcessIdValue value() const;

    void type(ProcessType type);
    ProcessType type() const;

    void num(ProcessNum num);
    ProcessNum num() const;

private:
    ProcessType m_type = 0;
    ProcessNum m_num = 0;

public:
    static std::string typeToString(ProcessType type);
    static ProcessType stringToType(const std::string& str);

private:
    friend class ProcessConfig;
    static std::vector<std::string> s_type2Name;
    static std::map<std::string, ProcessType> s_name2Type;

};

bool operator==(const ProcessId& pid1, const ProcessId& pid2);
bool operator!=(const ProcessId& pid1, const ProcessId& pid2);
bool operator<(const ProcessId& pid1, const ProcessId& pid2);

}}

#endif
