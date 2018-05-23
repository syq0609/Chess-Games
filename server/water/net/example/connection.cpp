#include "test.h"
#include "../connection.h"
#include "../http_packet.h"

#include <thread>

using namespace water;
using namespace net;

int main()
{
    cout << EINPROGRESS << endl;
    Endpoint ep("211.151.20.124:80");
    auto conn = TcpConnection::create(ep);
    int i = 0;
    while (!conn->tryConnect())
    {
        ++i;
    }
    cout << "try " << i << " times before connected" << endl;

    std::string requestFormat =
    "POST /api/User/LoginOauth/ HTTP/1.1\r\n"
    "Host: oauth.anysdk.com\r\n"
    "User-Agent: caonimaanysdk\r\n"
    "Accept: */*\r\n"
    "Content-Type: application/x-www-form-urlencoded\r\n"
    "Content-Length: {}\r\n"
    "\r\n"
    "{}";


    std::string body = "channel=iOS_wxpay&framework_version=IOS_CPP--_2.2.5&plugin_id=399&plugin_version=2.0.7_1.7.1&private_key=25B6DD68C7BE614BD320E80CE49F8BEA&server_ext_for_client=&server_ext_for_login=%7B%7D&server_id=1&token=071RvV141bSOVO1t9E041Sez141RvV10&uapi_key=7A48607F-9301-21AF-7D8A-2E9D586716EA&uapi_secret=f8360a3a2e34012f7efd2a75a6ced443";
    std::string request = componet::format(requestFormat, body.size(), body);

    auto ret1 = HttpPacket::tryParse(HttpMsg::Type::request, request.data(), request.size());
    if (ret1.second == false)
    {
        cout << "parse1 failed" << endl;
        return 0;
    }

    cout << request << endl;
    char buf[2024] = {0};
    conn->setBlocking();
    conn->send(ret1.first->data(), ret1.first->size());
    this_thread::sleep_for(std::chrono::seconds(2)); //
    conn->recv(buf, sizeof(buf));
    cout << buf << endl;
    return 0;
}
