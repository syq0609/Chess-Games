#include "string_kit.h"

namespace water{
namespace componet{

std::string dataToHex(const void* data, uint32_t size)
{
    auto buf = reinterpret_cast<const uint8_t*>(data);
    std::string ret;
    ret.reserve(size * 3);

    const char hex[] =
    {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
    };
    for (auto i = 0u; i < size; ++i)
    {
        if (i > 0 && i % 1 == 0)
            ret.push_back(' ');
        uint8_t tmp = buf[i];
        uint8_t tmp1 = tmp >> 4;
        uint8_t tmp2 = tmp & 0xf;
        ret.push_back(hex[tmp1]);
        ret.push_back(hex[tmp2]);
    }
    return ret;
}

std::string toString(const TimePoint& tp)
{
    return timePointToString(tp);
}

template<>
TimePoint fromString<TimePoint>(const std::string& str)
{
    return stringToTimePoint(str);
}

std::vector<std::string> splitString(const std::string& str, const std::string& delimiter)
{
    std::vector<std::string> result;
    splitString(&result, str, delimiter);
    return result;
}

void splitStr(std::vector<std::string>* ret, const std::string& str, const std::string& delimiter)
{
    splitString(ret, str, delimiter);
}


}}
