#include "net/connection.h"
//#include "net/packet_connection.h"
#include "net/buffered_connection.h"
#include "net/epoller.h"
#include "net/tcp_packet.h"
#include "base/tcp_message.h"
#include "componet/logger.h"
#include "componet/timer.h"
#include "componet/circular_queue.h"
#include "protocol/protobuf/proto_manager.h"

using namespace water;

using TcpMsgCode = process::TcpMsgCode;

const int32_t robotNum = 50;
const char cfgDir[] = "/home/airp/S/server/config";
const net::Endpoint serverAddr("127.0.0.1:7000");
//const net::Endpoint serverAddr("119.23.71.237:7000");

class Client
{
public:
    Client();
    void run();

    bool sendMsg(TcpMsgCode msgCode, const ProtoMsg& proto, uint32_t idx);
    ProtoMsgPtr recvMsg(TcpMsgCode msgCode, int32_t* idx);

    bool trySendMsg(TcpMsgCode msgCode, const ProtoMsg& proto, uint32_t idx);
    void registerRobotMsg();

private:
    void epollEventHandler(net::Epoller::Event event);
    void dealMsg(const componet::TimePoint& now, uint32_t idx);
    void regMsgHandler();
    ProtoMsgPtr tryRecvMsg(TcpMsgCode msgCode, uint32_t idx);

private:
    net::TcpPacket::Ptr m_recvPacket;
    //net::BufferedConnection::Ptr m_conn;
    net::Epoller m_epoller;
    componet::Timer m_timer;
    //componet::CircularQueue<net::Packet::Ptr> m_recvQue;
    //componet::CircularQueue<net::Packet::Ptr> m_sendQue;

    std::vector<TcpMsgCode> msgList;
    std::vector<net::BufferedConnection::Ptr> m_connList;
    std::vector<componet::CircularQueue<net::Packet::Ptr>> m_recvQueList;

public:
    static Client& me();
private:
    static Client s_me;
    net::Endpoint m_serverEp;
};

#define SEND_MSG(msgName, proto, idx) Client::me().sendMsg(PROTO_CODE_PUBLIC(msgName), proto, idx);
#define RECV_MSG(msgName, idx) PROTO_PTR_CAST_PUBLIC(msgName, Client::me().recvMsg(PROTO_CODE_PUBLIC(msgName), idx));
