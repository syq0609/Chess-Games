#include "../http_packet.h"

using namespace water;
using namespace net;

static const char data0[] =
    "POST /water/requesteample?num=1&code=aed HTTP/1.1\r\n"
    "Host: github.com\r\n"
    "DNT: 1\r\n"
    "Accept-Encoding: gzip, deflate, sdch\r\n"
    "Optional-test:\r\n"
    "Accept-Language: ru-RU,ru;q=0.8,en-US;q=0.6,en;q=0.4\r\n"
    "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_1) "
        "AppleWebKit/537.36 (KHTML, like Gecko) "
        "Chrome/39.0.2171.65 Safari/537.36\r\n";
static const char data1[] =
    "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,"
        "image/webp,*/*;q=0.8\r\n"
    "Referer: https://github.com/joyent/http-parser\r\n"
    "Connection: keep-alive\r\n"
    "Transfer-Encoding: chunked\r\n"
    "Cache-Control: max-age=0\r\n\r\n"
    "b\r\nhello world\r\n"
    "f\r\nin chunked body\r\n"
    "10\r\nin chunked body.\r\n"
    "0\r\n\r\n";
static const size_t data_len0 = sizeof(data0) - 1;
static const size_t data_len1 = sizeof(data1) - 1;

int main()
{
    HttpPacket packet(HttpType::request);
    packet.tryParse(data0, data_len0);
    packet.tryParse(data1, data_len1);
    return 0;
}

//g++ -std=c++14 -g  -c http-parser/http_parser.c -o http_parser.o &&  g++ -std=c++14 -g mytest.cpp http_parser.o
