#ifndef WATER_NET_HTTP_PACKET_H
#define WATER_NET_HTTP_PACKET_H


#include "packet.h"

#include <cstdint>
#include <string>
#include <map>


class http_parser;
class http_parser_settings;

namespace water{
namespace net{


struct HttpMsg
{
    enum class Type
    {
        request = 0, 
        response = 1,
    };


    Type type = Type::request;
    std::string curHeaderFiled;
    std::map<std::string, std::string> headers;
    union
    {
        uint32_t statusCode = 0;
        uint32_t method;
    };
    bool keepAlive = false;
    std::string body;
    std::string url;
};

class HttpPacket : public Packet
{
    using Parser = http_parser;
public:
    TYPEDEF_PTR(HttpPacket)
    CREATE_FUN_MAKE(HttpPacket)
    HttpPacket(HttpMsg::Type type);
    ~HttpPacket();

    bool complete() const;
    HttpMsg::Type msgType() const;
    bool keepAlive() const;
    HttpMsg& msg();

    size_t parse(const char* data, size_t size);

private:
    std::unique_ptr<http_parser> m_parser;
    bool m_completed = false;
    HttpMsg m_msg;

public:
    static std::pair<HttpPacket::Ptr, size_t> tryParse(HttpMsg::Type type, const char* data, size_t size);

private:
    struct ParserSettings;
    static std::unique_ptr<http_parser_settings> s_settings;

    static int32_t onPacketBegin(Parser* parser);
    static int32_t onUrl(Parser* parser, const char* data, size_t size);
    static int32_t onStatus(Parser* parser, const char* data, size_t size);
    static int32_t onHeaderField(Parser* parser, const char* data, size_t size);
    static int32_t onHeaderValue(Parser* parser, const char* data, size_t size);
    static int32_t onHeadersComplete(Parser* parser);
    static int32_t onBody(Parser* parser, const char* data, size_t size);
    static int32_t onPacketComplete(Parser* parser);
    static int32_t onChunkHeader(Parser* parser);
    static int32_t onChunkComplete(Parser* parser);
};

inline bool HttpPacket::complete() const
{
    return m_completed;
}

inline HttpMsg::Type HttpPacket::msgType() const
{
    return m_msg.type;
}

inline bool HttpPacket::keepAlive() const
{
    return m_msg.keepAlive;
}

inline HttpMsg& HttpPacket::msg()
{
    return m_msg;
}

}}


#endif

