/*
 * Author: LiZhaojia - waterlzj@gmail.com
 *
 * Last modified: 2017-06-18 04:16 +0800
 *
 * Description: AnySdk 接入, 服务器支持
 *
 *              这东西难用的要死, 文档严重不全, 通信协议的格式完全没有文档保证, 
 *              给出的github上的所谓gameserver端的demo竟然是代码片段不支持运行(虽然改为可运行也不难)
 *              客户端在集成后很高概率会导致打包失败或启动崩溃,
 *              社区的存在大量挂了数个甚至数十个月没人理的求助帖, 官方一个产品经理在zhihu上态度非常恶劣
 *              因某些原因本次只能继续接下去, 包格式就hack解决了,
 *              最好永远不要再碰到这个产品以及这家公司的所有产品并组织其它人使用(端开发者的意见完全一直)
 *              以下将AnySdkServer简写为ass
 */




#ifndef GATEWAY_ANYSDK_LOGIN_MANAGER_H
#define GATEWAY_ANYSDK_LOGIN_MANAGER_H

#include "net/buffered_connection.h"

#include "componet/fast_travel_unordered_map.h"
#include "componet/spinlock.h"
#include "componet/datetime.h"

#include "base/process_id.h"

#include <atomic>
#include <list>


namespace water{ namespace net{ class HttpPacket; } }

namespace gateway{

using namespace water;
using namespace process;

class AnySdkLoginManager
{
public:
    AnySdkLoginManager();
    ~AnySdkLoginManager() = default;


    //新的http连接呼入
    void onNewHttpConnection(net::BufferedConnection::Ptr conn);

    //启一个线程, 定时的做anysdk server的域名解析, 
    //做法很山寨, getAddrInfo_a这个函数基本不能用, 对着RFC标准自己写异步太折腾, 就先这样子吧
    void startNameResolve();
    
    //登陆令牌验证
    bool checkAccessToken(const std::string& openid, const std::string& token) const;

    //删除过期的token
    void timerExec(componet::TimePoint now);

    //事件响应, 订阅 HttpConnMgr 的 afterEraseConn事件
    void afterClientDisconnect(HttpConnectionId hcid);

private:
    //包处理定时器
    void dealHttpPackets(componet::TimePoint now);

    //得到一个新的hcid
    HttpConnectionId genCliHttpConnectionId() const;

    //随机一个访问Token, 时间戳+随机32bits整数, 放在一起搞成hex格式
    std::string genAccessToken();

    void eraseLater(HttpConnectionId hcid);

    bool isAssHcid(HttpConnectionId hcid);
    bool isCliHcid(HttpConnectionId hcid);


private:
    struct TokenInfo
    {
        CREATE_FUN_MAKE(TokenInfo)
        TYPEDEF_PTR(TokenInfo)

        std::string openid;
        std::string token;
        time_t expiry = 0;
    };
    componet::FastTravelUnorderedMap<std::string, TokenInfo::Ptr> m_tokens;

    mutable HttpConnectionId m_lastHcid = 1;
    net::BufferedConnection::Ptr m_connToAss;

    struct AssIpInfo
    {
        mutable componet::Spinlock lock;
        std::string ipstr; //ass 验证服务器的ip
    } m_assip;

    struct AllClients
    {
        struct AnySdkClient
        {
            TYPEDEF_PTR(AnySdkClient)
            CREATE_FUN_MAKE(AnySdkClient)
            enum class Status
            {
                recvCliReq  = 0, //recv request from client
                connToAss   = 1, //connect to ass
                reqToAss    = 2, //send request to ass
                recvAssRsp  = 3, //recv response from ass
                rspToCli    = 4, //send respnese to client
                done        = 5, //every step finished without any errors
                assAbort    = 6, //error caused by something about ass, need abort
                cliAbort    = 7, //error caused by something abort client, need abort
                destroy     = 8, //corot has exited
            };
            Status status = Status::recvCliReq;
            HttpConnectionId clihcid = 0;
            std::shared_ptr<net::HttpPacket> cliReq;
            std::shared_ptr<net::HttpPacket> assRsp;

            const AssIpInfo* assip = nullptr;
            componet::FastTravelUnorderedMap<std::string, TokenInfo::Ptr>* tokens = nullptr;
            componet::TimePoint* now = nullptr;
            void corotExec();
        };
        std::unordered_map<HttpConnectionId, AnySdkClient::Ptr> corotClients;

        //新到的client, 由httpserver线程添加, 然后由主定时器线程中处理, 所以需要锁
        componet::Spinlock newClentsLock;
        componet::FastTravelUnorderedMap<HttpConnectionId, AnySdkClient::Ptr> newClients;
    } m_allClients;


    componet::Spinlock m_closedHcidsLock;
    std::list<HttpConnectionId> m_closedHcids;

    componet::TimePoint m_now;
    
private:
    static AnySdkLoginManager s_me;
public:
    static AnySdkLoginManager& me();
};


}//end namespace gateway

#endif
