#include "http_packet.h"

#include "http-parser/http_parser.h"

#include "componet/logger.h"

#include <iostream>
using std::cout;
using std::endl;


//设计, 不用典型的ringbuf, 线性buf, 因为ringbuf在接缝处处理时, 还是需要memcpy, 没太大意义
//遇到buf结尾, 把未解析的数据memmove到buf头部, 然后继续
namespace water{
namespace net{

struct HttpPacket::ParserSettings : http_parser_settings
{
    ParserSettings()
    {
        on_message_begin      = HttpPacket::onPacketBegin     ;
        on_url                = HttpPacket::onUrl             ;
        on_status             = HttpPacket::onStatus          ;
        on_header_field       = HttpPacket::onHeaderField     ;
        on_header_value       = HttpPacket::onHeaderValue     ;
        on_body               = HttpPacket::onBody            ;
        on_headers_complete   = HttpPacket::onHeadersComplete ;
        on_message_complete   = HttpPacket::onPacketComplete  ;
        on_chunk_header       = HttpPacket::onChunkHeader     ;
        on_chunk_complete     = HttpPacket::onChunkComplete   ;
    }
};


std::unique_ptr<http_parser_settings> HttpPacket::s_settings(new ParserSettings);


std::pair<HttpPacket::Ptr, size_t> HttpPacket::tryParse(HttpMsg::Type type, const char* data, size_t size)
{
    auto ret = HttpPacket::create(type);
    size_t used = ret->parse(data, size);
    return {ret->m_completed ? ret : nullptr, used};
}

int32_t HttpPacket::onPacketBegin(Parser* parser)
{
    return 0;
}

int32_t HttpPacket::onUrl(Parser* parser, const char* data, size_t size)
{
    cout << "url=" << std::string(data, size) << endl;
    auto packet = reinterpret_cast<HttpPacket*>(parser->data);
    packet->m_msg.url.assign(data, size);
    return 0;
}

int32_t HttpPacket::onStatus(Parser* parser, const char* data, size_t size)
{
    cout << "status, code=" << parser->status_code << endl;
    cout << "status, method=" << parser->method << endl;
    auto packet = reinterpret_cast<HttpPacket*>(parser->data);
    if (packet->m_msg.type == HttpMsg::Type::request)
        packet->m_msg.method = parser->method;
    else
        packet->m_msg.statusCode = parser->status_code;
    return 0;
}

int32_t HttpPacket::onHeaderField(Parser* parser, const char* data, size_t size)
{
    cout << "headerFiled=" << std::string(data, size) << endl;
    auto packet = reinterpret_cast<HttpPacket*>(parser->data);
    packet->m_msg.curHeaderFiled.assign(data, size);
    return 0;
}

int32_t HttpPacket::onHeaderValue(Parser* parser, const char* data, size_t size)
{
    cout << "headerValue=" << std::string(data, size) << endl;
    auto packet = reinterpret_cast<HttpPacket*>(parser->data);
    packet->m_msg.headers[packet->m_msg.curHeaderFiled] = std::string(data, size);
    return 0;
}

int32_t HttpPacket::onHeadersComplete(Parser* parser)
{
    auto packet = reinterpret_cast<HttpPacket*>(parser->data);
    cout << "headerEnd, size=" << packet->m_msg.headers.size() << endl;
    std::string().swap(packet->m_msg.curHeaderFiled);
    return 0;
}

int32_t HttpPacket::onBody(Parser* parser, const char* data, size_t size)
{
    cout << "body: " << std::string(data, size) << endl;
    auto packet = reinterpret_cast<HttpPacket*>(parser->data);
    packet->m_msg.body.append(data, size);
    return 0;
}

int32_t HttpPacket::onPacketComplete(Parser* parser)
{
    cout << "packet complete" << endl;
    auto packet = reinterpret_cast<HttpPacket*>(parser->data);
    packet->m_completed = true;
    packet->m_msg.body.shrink_to_fit();
    http_parser_pause(parser, 1);
    return 0;
}

int32_t HttpPacket::onChunkHeader(Parser* parser)
{
    return 0;
}

int32_t HttpPacket::onChunkComplete(Parser* parser)
{
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
HttpPacket::HttpPacket(HttpMsg::Type type)
    : m_parser(new http_parser)
{
    http_parser_init(m_parser.get(), type == HttpMsg::Type::request ? HTTP_REQUEST : HTTP_RESPONSE);
    m_parser->data = this;
    m_msg.type = type;
}

HttpPacket::~HttpPacket() = default;


size_t HttpPacket::parse(const char* data, size_t size)
{
    if (complete())
        return 0;
    http_parser_pause(m_parser.get(), 0);
    size_t ret = http_parser_execute(m_parser.get(), s_settings.get(), data, size);
    append(data, ret);
    return ret;
}



}}

