#include "../buffered_connection.h"
#include "../tcp_packet.h"
#include "../../componet/string_kit.h"

#include <iostream>

using namespace water;
using namespace net;
using namespace componet;

using std::cout;
using std::endl;

TcpPacket packets[20];

std::string data[11] = 
{
    "123456",
    "12345678",
    "123456789",
    "1234567890",
    "1234567890a",
    "1234567890ab",
    "1234567890abcd",
    "1234567890abcde",
    "1234567890abcdef",
    "1234567890abcdafg",
    "1234567890abcdefgh",
};

int main()
{
    uint32_t uuu = 45;
    cout << 0x29 << endl;
    cout << dataToHex(&uuu, sizeof(uuu)) << endl;

    uint64_t t[2] = {0x1234567890abcdef, 0x1234567890abcdef};
    cout << sizeof(t) << endl;
    cout << dataToHex(((char*)t) + 8, 8) << endl;
    cout << dataToHex(t, sizeof(t)) << endl;

    std::string buf;
    for(uint32_t i = 0; i < 20; ++i)
    {
        auto j = i % 11u;
        packets[i].setContent(data[j].data(), data[j].size());
        buf.append(packets[i].data(), packets[i].size());
    }
    /*
    cout << buf << endl;
    const char* cbuf = buf.data();
    std::vector<TcpPacket*> vec;
    auto p = new TcpPacket();
    uint32_t front = 0;
    while (front < buf.size())
    {
        auto reserve = buf.size() - front;
        auto feed = reserve < 10 ? reserve : 10;
        auto used = p->parse(buf.data() + front, feed);
        front += used;
        if (p->complete())
        {
            vec.push_back(p);
            p = new TcpPacket();
        }
    }

    if (vec.size() != 20)
    {
        cout << "vec.size()=" << vec.size() <<endl;
        return 0;
    }

    for (auto i = 0u; i < 20; ++i)
    {
        auto str1 = std::string(packets[i].data(), packets[i].size());
        auto str2 = std::string(vec[i]->data(), vec[i]->size());

        if (str1 != str2)
            cout << "deff " << i << ", data=" << str2 << endl;
    }
*/

    ConnectionBuffer in(20);
    ConnectionBuffer out(20);

    std::string msg = "abcdefghijklmnopqrstuvwxyz";
    auto inW = in.writeable();
    auto len = msg.copy(inW.first, inW.second);
    in.commitWrite(len);

    auto inR = in.readable();
    for (auto i = 0u; i < inR.second; ++i)
        cout << inR.first[i];
    cout << endl;

    in.commitRead(6);
    inR = in.readable();
    for (auto i = 0u; i < inR.second; ++i)
        cout << inR.first[i];
    cout << endl;
}









