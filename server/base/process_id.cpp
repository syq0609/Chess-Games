#include "process_id.h"

#include "process_config.h"
#include "componet/format.h"



namespace water{
namespace process{

std::vector<std::string> ProcessId::s_type2Name = {"none"};
std::map<std::string, ProcessType> ProcessId::s_name2Type;

std::string ProcessId::typeToString(ProcessType type)
{
    uint32_t index = static_cast<uint32_t>(type);
    if(index >= s_type2Name.size())
        return "none";

    return s_type2Name[index];
}

ProcessType ProcessId::stringToType(const std::string& str)
{
    auto it = s_name2Type.find(str);
    if(it == s_name2Type.end())
        return INVALID_PROCESS_TYPE;

    return it->second;
}

ProcessId::ProcessId(const std::string& typeStr, int8_t num)
{
    ProcessType type = ProcessId::stringToType(typeStr);
    if(type == INVALID_PROCESS_TYPE)
    {
        m_type = 0;
        m_num = 0;
        return;
    }

    m_type   = type;
    m_num    = num;
}

ProcessId::ProcessId(ProcessIdValue value)
{
    setValue(value);
}

std::string ProcessId::toString() const
{
    std::string ret = typeToString(m_type);
    componet::formatAndAppend(&ret, "-{}", m_num);
    return ret;
}

void ProcessId::clear()
{
    m_type = 0;
    m_num = 0;
}

bool ProcessId::isValid() const
{
    return typeToString(m_type) != "none" && m_num != 0;
}

ProcessIdValue ProcessId::value() const
{
    return (ProcessIdValue(0) << 16u) | (m_type << 8u) | m_num;
}

void ProcessId::setValue(ProcessIdValue value)
{
    m_type   = (value & 0xff00) >> 8u;
    m_num    = value & 0xff;
}

void ProcessId::type(ProcessType type) 
{
    m_type = type;
}

ProcessType ProcessId::type() const
{
    return m_type;
}

void ProcessId::num(ProcessNum num)
{
    m_num = num;
}

ProcessNum ProcessId::num() const
{
    return m_num;
}

/////////////////////////////////////////////////////////////////////
bool operator==(const ProcessId& pid1, const ProcessId& pid2)
{
    return pid1.value() == pid2.value();
}

bool operator!=(const ProcessId& pid1, const ProcessId& pid2)
{
    return pid1.value() != pid2.value();
}

bool operator<(const ProcessId& pid1, const ProcessId& pid2)
{
    return pid1.value() < pid2.value();
}


}}

