#include "client_manager.h"
#include "client.h"
#include "lobby.h"
#include "game_config.h"
#include "room.h"

#include "dbadaptcher/redis_handler.h"

#include "componet/logger.h"
#include "componet/scope_guard.h"
#include "componet/string_kit.h"
#include "componet/http_request.h"
#include "componet/url_encrypt.h"
#include "componet/json.h"

#include "protocol/protobuf/public/client.codedef.h"
#include "protocol/protobuf/private/login.codedef.h"


namespace lobby{


static const char* MAX_CLIENT_UNIQUE_ID_NAME = "max_client_unique_id";

/***********************************************/

ClientManager* ClientManager::s_me = nullptr;

ClientManager& ClientManager::me()
{
    if(s_me == nullptr)
        s_me = new ClientManager();
    return *s_me;
}

////////////////////

void ClientManager::init()
{
    recoveryFromRedis();
}

ClientUniqueId ClientManager::getClientUniqueId()
{
    const ClientUniqueId nextCuid = m_uniqueIdCounter + 1;

    using water::dbadaptcher::RedisHandler;
    RedisHandler& redis = RedisHandler::me();
    if (!redis.set(MAX_CLIENT_UNIQUE_ID_NAME, componet::format("{}", nextCuid)))
        return INVALID_CUID;
    return ++m_uniqueIdCounter;
}

void ClientManager::recoveryFromRedis()
{
    using water::dbadaptcher::RedisHandler;
    RedisHandler& redis = RedisHandler::me();
    std::string maxUinqueIdStr = redis.get(MAX_CLIENT_UNIQUE_ID_NAME);
    if (!maxUinqueIdStr.empty())
    {
        m_uniqueIdCounter = atoll(maxUinqueIdStr.c_str());
        LOG_TRACE("ClientManager启动, 初始化, 加载m_uniqueIdCounter={}", m_uniqueIdCounter);
    }
    else
    {
        LOG_TRACE("ClientManager启动, 初始化, 默认m_uniqueIdCounter={}", m_uniqueIdCounter);
    }

    return;


    auto exec = [this](const std::string openid, const std::string& bin) -> bool
    {
        auto client = Client::create();
        if (!client->deserialize(bin))
        {
            LOG_ERROR("recovery clients from reids, deserialize client failed, openid={}", openid);
            return true;
        }
        if (!insert(client))
        {
            LOG_ERROR("recovery clients from reids, insert client failed, openid={}, cuid={}, name={}",
                      client->openid(), client->cuid(), client->name());
        }
        return true;
    };
    redis.htraversal(CLIENT_TABLE_BY_OPENID, exec);
    LOG_TRACE("recovery clients data from redis");
}

void ClientManager::timerExecAll(componet::TimePoint now)
{
    const auto& shareByWeChatCfg = GameConfig::me().data().shareByWeChat;
    bool shareByWeChatStatusChanged = false;
    if (Client::ShareByWeChat::isActive != (shareByWeChatCfg.begin <= now && now < shareByWeChatCfg.end))
    {
        shareByWeChatStatusChanged = true;
        Client::ShareByWeChat::isActive = !Client::ShareByWeChat::isActive;
    }

    std::vector<Client::Ptr> clientsOfflineLongTimeAgo;
    for (auto iter = m_cuid2Clients.begin(); iter != m_cuid2Clients.end(); ++iter)
    {
        auto client = iter->second;
        if (client->roomid() == 0 &&
            client->offlineTime() != componet::EPOCH && client->offlineTime() + std::chrono::seconds(600) > now)
        {
            clientsOfflineLongTimeAgo.push_back(client);
        }

        //在线
        if (client->offlineTime() == componet::EPOCH)
        {
            if (shareByWeChatStatusChanged)
            {
                client->syncShareByWeChatStatus();
            }
        }
    }

    for (auto client : clientsOfflineLongTimeAgo)
    {
        LOG_TRACE("logout, 客户端离线时间过长删除, openid={}, cuid={}, name={}", client->openid(), client->cuid(), client->name());
        erase(client);
    }
}

bool ClientManager::sendToClient(ClientConnectionId ccid, TcpMsgCode code, const ProtoMsg& proto)
{
    ProcessId gatewayPid("gateway", 1);
    return Lobby::me().relayToPrivate(ccid, gatewayPid, code, proto);
}

void ClientManager::rechargeMoney(uint32_t sn, ClientUniqueId cuid, int32_t money, const std::string& theOperator)
{
    auto client = getByCuid(cuid);
    if (client == nullptr)
    {
        auto loadRet = loadClient(cuid);
        if (!loadRet.second)
            LOG_ERROR("recharge failed, redis load failed, sn={}, cuid={}, money={}, theOperator={}", sn, cuid, money, theOperator);
        client = loadRet.first;
    }
    if (client == nullptr)
    {
        LOG_ERROR("recharge failed, cuid not exisit, sn={}, cuid={}, money={}, theOperator={}", sn, cuid, money, theOperator);
        return;
    }

    auto newMoney = client->addMoney(money);
    client->syncBasicDataToClient();
    LOG_TRACE("recharge successed, sn={}, cuid={}, money={}, newMoney={}, theOperator={}", sn, cuid, money, newMoney, theOperator);
}

bool ClientManager::insert(Client::Ptr client)
{
    if(client == nullptr)
        return false;

    if (!m_openid2Clients.insert(client->openid(), client))
        return false;
    if (!m_cuid2Clients.insert(client->cuid(), client))
    {
        m_openid2Clients.erase(client->openid());
        return false;
    }
    if (!m_ccid2Clients.insert(client->ccid(), client))
    {
        m_openid2Clients.erase(client->openid());
        m_cuid2Clients.erase(client->cuid());
        return false;
    }
   return true;
}

void ClientManager::erase(Client::Ptr client)
{
    m_openid2Clients.erase(client->openid());
    m_cuid2Clients.erase(client->cuid());
    m_ccid2Clients.erase(client->ccid());
}

Client::Ptr ClientManager::getByCcid(ClientConnectionId ccid) const
{
    auto iter = m_ccid2Clients.find(ccid);
    if (iter == m_ccid2Clients.end())
        return nullptr;
    return iter->second;
}

Client::Ptr ClientManager::getByCuid(ClientUniqueId cuid) const
{
    auto iter = m_cuid2Clients.find(cuid);
    if (iter == m_cuid2Clients.end())
        return nullptr;
    return iter->second;
}

Client::Ptr ClientManager::getByOpenid(const std::string& openid) const
{
    auto iter = m_openid2Clients.find(openid);
    if (iter == m_openid2Clients.end())
        return nullptr;
    return iter->second;
}

bool ClientManager::isOnline(ClientUniqueId cuid) const
{
    auto client = getByCuid(cuid);
    if (client == nullptr)
        return false;
    return client->offlineTime() == componet::EPOCH;
}

std::pair<Client::Ptr, bool> ClientManager::loadClient(const std::string& openid)
{
    using water::dbadaptcher::RedisHandler;
    RedisHandler& redis = RedisHandler::me();
    const std::string& bin = redis.hget(CLIENT_TABLE_BY_OPENID, openid);
    if (bin == "")
        return {nullptr, true};

    auto client = Client::create();
    if( !client->deserialize(bin) )
    {
        LOG_ERROR("load client from redis, deserialize failed, openid={}", openid);
        return {nullptr, false};
    }
    LOG_TRACE("load client, successed, openid={}, cuid={}, roomid={}", client->openid(), client->cuid(), client->roomid());
    return {client, true};
}

std::pair<Client::Ptr, bool> ClientManager::loadClient(ClientUniqueId cuid)
{
    using water::dbadaptcher::RedisHandler;
    RedisHandler& redis = RedisHandler::me();
    const std::string& openid = redis.hget(CLIENT_CUID_2_OPENID, componet::toString(cuid));
    if (openid == "")
    {
        LOG_ERROR("load client from redis, get openid by cuid failed, cuid={}", cuid);
        return {nullptr, false};
    }
    return loadClient(openid);
}

bool ClientManager::saveClient(Client::Ptr client)
{
    if (client == nullptr)
        return false;

    using water::dbadaptcher::RedisHandler;
    RedisHandler& redis = RedisHandler::me();

    const std::string& openid = redis.hget(CLIENT_CUID_2_OPENID, componet::toString(client->cuid()));
    if (openid == "")
    {
        if (!redis.hset(CLIENT_CUID_2_OPENID, componet::toString(client->cuid()), client->openid()))
        {
            LOG_ERROR("save cuid_2_openid failed, cuid={}, openid={}", client->cuid(), client->openid());
            return false;
        }
        if (!client->saveToDB())
        {
            redis.hdel(CLIENT_CUID_2_OPENID, componet::toString(client->cuid()));
            return false;
        }
        return true;
    }

    return client->saveToDB();
}

void ClientManager::proto_LoginQuest(ProtoMsgPtr proto, ProcessId gatewayPid)
{
    //login step2, 获取用户当前状态， 如果是新用户则需要加入数据库
    auto rcv = PROTO_PTR_CAST_PRIVATE(LoginQuest, proto);
    const ClientUniqueId ccid = rcv->ccid();
    const std::string openid = rcv->openid();
    LOG_TRACE("C_Login, ccid={}, openid={}", ccid, openid);


    PrivateProto::RetLoginQuest retMsg;
    TcpMsgCode retCode = PROTO_CODE_PRIVATE(RetLoginQuest);
    retMsg.set_ccid(ccid);
    retMsg.set_openid(openid);
    //RAII的方式在离开函数时, 自动发送登陆结果
    componet::ScopeGuard autoSendLogRet([&retMsg, &gatewayPid, retCode]()
                              {
                                  Lobby::me().sendToPrivate(gatewayPid, retCode, retMsg);
                              });

    Client::Ptr client = getByOpenid(openid);
    if (client == nullptr) //不在线
    {
        const auto& loadRet = loadClient(openid);
        if (!loadRet.second)
        {
            retMsg.set_ret_code(PrivateProto::RLQ_REG_FAILED);
            LOG_ERROR("login, step 2, client load failed, openid={}", openid);
            return;
        }
        client = loadRet.first;
        if(client == nullptr) //新用户
        {
            if (!rcv->verified())
            {
                retMsg.set_ret_code(PrivateProto::RLQ_REG_FAILED);
                LOG_ERROR("login, step 2, failed,  new client but not from wechat, openid={}", openid);
                return;
            }
            client = Client::create();
            client->m_ccid = ccid;
            client->m_cuid = getClientUniqueId();
            client->m_openid = openid;
            client->m_name = rcv->name();
            client->m_token = rcv->token();
            client->m_sex = rcv->sex();

            if (!insert(client))
            {
                retMsg.set_ret_code(PrivateProto::RLQ_REG_FAILED);
                LOG_ERROR("login, step 2, new client reg failed, ccid={}, cuid={}, openid={}", client->ccid(), client->cuid(), openid);
                return;
            }
            LOG_TRACE("login, step 2, new client reg successed, ccid={}, cuid={}, openid={}", client->ccid(), client->cuid(), openid);
        }
        else //登陆过但不在线
        {
            if ( !rcv->verified() && rcv->token() != client->m_token )
            {
                retMsg.set_ret_code(PrivateProto::RLQ_TOKEN_EXPIRIED);
                LOG_TRACE("login, step 2, old client online failed, ilegal token, openid={}, oldtoken={}, rcvtoken={}", 
                          client->openid(), client->m_token, rcv->token());
                return;
            }
            client->m_token = rcv->token();
            client->m_ccid = ccid;
            client->m_sex = rcv->sex();
            if (!insert(client))
            {
                retMsg.set_ret_code(PrivateProto::RLQ_REG_FAILED);
                LOG_ERROR("login, step 2, old client online failed, ccid={}, cuid={}, openid={}", client->ccid(), client->cuid(), client->openid());
                return;
            }
            LOG_TRACE("login, step 2, old client online successed, ccid={}, cuid={}, openid={}", client->ccid(), client->cuid(), openid);
        }
    }
    else  //本来就在线
    {
        if ( !rcv->verified() && rcv->token() != client->m_token )
        {
            retMsg.set_ret_code(PrivateProto::RLQ_TOKEN_EXPIRIED);
            LOG_TRACE("login, step 2, old client replaced failed, ilegal token, openid={}, oldtoken={}, rcvtoken={}", 
                      client->openid(), client->m_token, rcv->token());
            return;
        }
        
        client->noticeMessageBox("本账号在其它终端上登录, 你已下线");

        //更新token
        client->m_token = rcv->token();

        //TODO modify, 发送消息给gw，让老的连接下线
        PrivateProto::ClientBeReplaced cbr;
        TcpMsgCode cbrCode = PROTO_CODE_PRIVATE(ClientBeReplaced);
        cbr.set_cuid(client->cuid());
        cbr.set_ccid(client->ccid());
        cbr.set_openid(client->openid());
        Lobby::me().sendToPrivate(gatewayPid, cbrCode, cbr);
        LOG_TRACE("login, step 2.5, client was replaced, ccid={}, cuid={}, openid={}, newccid={}", client->ccid(), client->cuid(), client->openid(), ccid);

        //更新ccid 和 name
        erase(client);
        client->m_ccid = ccid;
        client->m_name = rcv->name();
        client->m_sex  = rcv->sex();
        if (!insert(client))
        {
            //更新ccid失败, 登陆失败
            retMsg.set_ret_code(PrivateProto::RLQ_REG_FAILED);
            LOG_TRACE("login, step 2, new client reg failed, ccid={}, cuid={}, openid={}", client->ccid(), client->cuid(), client->openid());
            return;
        }
    }

    client->m_token = rcv->token();
    client->m_ccid  = rcv->ccid();
    client->m_name  = rcv->name();
    client->m_imgurl= rcv->imgurl();
    client->m_ipstr = rcv->ipstr();
    client->m_sex   = rcv->sex();
    if (rcv->verified() && !saveClient(client))
    {
        erase(client);
        retMsg.set_ret_code(PrivateProto::RLQ_REG_FAILED);
        LOG_ERROR("login, step 2, new client, saveClient failed, openid={}", openid);
        return;
    }

    //登陆成功
    autoSendLogRet.dismiss(); //取消RAII的自动发送
    retMsg.set_ret_code(PrivateProto::RLQ_SUCCES);
    retMsg.set_cuid(client->cuid());
    retMsg.set_openid(client->openid());
    retMsg.set_roomid(client->roomid());
    retMsg.set_nick_name(client->name());
    retMsg.set_imgurl(client->imgurl());
    retMsg.set_sex(client->sex());
    Lobby::me().sendToPrivate(gatewayPid, retCode, retMsg);
    LOG_TRACE("login, step 2, 读取或注册client数据成功, ccid={}, cuid={}, openid={}, roomid={}", ccid, client->cuid(), client->openid(), client->roomid());

    //处理上线事件
    client->online();
    Room::clientOnline(client);

    return;
}

void ClientManager::proto_ClientDisconnected(ProtoMsgPtr proto, ProcessId gatewayPid)
{
    auto rcv = PROTO_PTR_CAST_PRIVATE(ClientDisconnected, proto);
    Client::Ptr client = getByCcid(rcv->ccid());
    if (client == nullptr)
        return;

    client->offline();

    LOG_TRACE("logout, 网关通知玩家离线, ccid={}, cuid={}, openid={}, roomid={}", client->ccid(), client->cuid(), client->openid(), client->roomid());
}

void ClientManager::proto_C_SendChat(const ProtoMsgPtr& proto, ClientConnectionId ccid)
{
    auto rcv = PROTO_PTR_CAST_PUBLIC(C_SendChat, proto);
    Client::Ptr client = getByCcid(ccid);
    if (client == nullptr)
        return;

    auto room = Room::get(client->roomid());
    if (room == nullptr)
        return;

    PROTO_VAR_PUBLIC(S_Chat, snd);
    snd.set_cuid(client->cuid());
    auto sndCtn = snd.mutable_content();
    sndCtn->set_type(rcv->type());
    switch (rcv->type())
    {
    case PublicProto::CHAT_TEXT:
        {
            sndCtn->set_data_text(rcv->data_text());
        }
        break;
    case PublicProto::CHAT_FACE:
    case PublicProto::CHAT_VOICE:
        {
            sndCtn->set_data_int(rcv->data_int());
            sndCtn->set_data_text(rcv->data_text());
        }
        break;
    }
    room->sendToOthers(client->cuid(), sndCode, snd);
}

void ClientManager::proto_C_G13_ReqGameHistoryCount(ClientConnectionId ccid)
{
    Client::Ptr client = getByCcid(ccid);
    if (client == nullptr)
        return;

    PROTO_VAR_PUBLIC(S_G13_GameHistoryCount, snd);
    snd.set_total(client->m_g13his.details.size());
    snd.set_week_rank(client->m_g13his.weekRank);
    snd.set_week_game(client->m_g13his.weekGame);
    snd.set_today_rank(client->m_g13his.todayRank);
    snd.set_today_game(client->m_g13his.todayGame);
    sendToClient(ccid, sndCode, snd);
}

void ClientManager::proto_C_G13_ReqGameHistoryDetial(const ProtoMsgPtr& proto, ClientConnectionId ccid)
{
    auto rcv = PROTO_PTR_CAST_PUBLIC(C_G13_ReqGameHistoryDetial, proto);
    Client::Ptr client = getByCcid(ccid);
    if (client == nullptr)
        return;

    const uint32_t perPage  = 5;
    const uint32_t total    = client->m_g13his.details.size();
    int32_t maxPage = 0;
    if (total > 0)
        maxPage = (total - 1) / perPage;
    const uint32_t page = rcv->page() <= maxPage ? rcv->page() : maxPage;

    uint32_t first = perPage * page;
    if (first != 0 && first >= total)
        first = perPage * (page - 1);

    PROTO_VAR_PUBLIC(S_G13_GameHistoryDetial, snd);
    uint32_t index = 0;
    for (const auto&detail : client->m_g13his.details)
    {
        if (index++ < first)           //first之前的跳过
            continue;
        if (index > first + perPage)   //从first开始, 只读一页的条目数, 注意, 上面++执行过, 所以这里判>而不是>=
            break;

        auto item = snd.add_items();
        item->set_roomid(detail->roomid);
        item->set_rank(detail->rank);
        item->set_time(detail->time);

        for (const auto& opp : detail->opps)
        {
            auto sndOpp = item->add_opps();
            sndOpp->set_cuid(opp.cuid);
            sndOpp->set_name(opp.name);
            sndOpp->set_rank(opp.rank);
        }
    }
    sendToClient(ccid, sndCode, snd);
}

void ClientManager::proto_C_G13_OnShareAdvByWeChat(ClientConnectionId ccid)
{
    Client::Ptr client = getByCcid(ccid);
    if (client == nullptr)
        return;
    client->afterShareByWeChat();
}

void ClientManager::proto_C_G13_Payment(const ProtoMsgPtr& proto, ClientConnectionId ccid)
{
	auto rcv = PROTO_PTR_CAST_PUBLIC(C_G13_Payment, proto);
	Client::Ptr client = getByCcid(ccid);
	if (client == nullptr)
		return;

    std::string sid = rcv->sid();
    auto iosPayment = GameConfig::me().data().iosPayment;

    if(iosPayment.find(sid) == iosPayment.end())
    {
        return;
    }

	int res = 0;
    int32_t money = 0;
	std::string data = rcv->data();
    if(GameConfig::me().data().iosPaymentSwitch == true)
    {
        if(data == "win32")     // TODO 临时给客户端充钱用
        {
            money = iosPayment[sid].base;
        }
        else if(checkForHttpPost(data) == 0)
        {
            money = iosPayment[sid].base;
        }
    }
    else
    {
        //其他支付平台验证
        if(false)
        {
            money = iosPayment[sid].base + iosPayment[sid].award;
        }
    }

    if(money != 0)
    {
        client->addMoney(money);
        res = 1;
    }

	PROTO_VAR_PUBLIC(S_G13_PaymentRes, snd);
	snd.set_result(res);
	sendToClient(ccid, sndCode, snd);
}

int ClientManager::checkForHttpPost(std::string data)
{
	char ios_check_url[] = "127.0.0.1/ios.php";
    componet::HttpRequest http;
	char http_return[4096] = {0};
	char http_msg[4096] = {0};

	std::string result = componet::urlEncode(data);
	char params[10240] = "receipt=";
	strcat(params, result.c_str());
	strcpy(http_msg, ios_check_url);

	if(http.HttpPost(http_msg, params, http_return)){
		std::string ret = http_return;
		std::size_t idx = ret.find("{");
		if(idx != std::string::npos)
		{
			std::size_t end = ret.find_last_of("}");
			if(end != std::string::npos)
			{
				//std::cout << ret.substr(idx, end - idx + 1) << "\n------------------\n" << std::endl;
				std::string jsonObj = ret.substr(idx, end - idx + 1);
                auto j = componet::json::parse(jsonObj);
                if(!j["status"].is_null() && j["status"].is_number())
                {
                    return j["status"].get<int>();
                }
			}
		}
	}
	return -1;
}

void ClientManager::proto_C_Exchange(const ProtoMsgPtr& proto, ClientConnectionId ccid)
{
	auto rcv = PROTO_PTR_CAST_PUBLIC(C_Exchange, proto);
	Client::Ptr client = getByCcid(ccid);
	if (client == nullptr)
		return;

	int res = 0;
    std::string key = rcv->key();
    auto goldExchange = GameConfig::me().data().goldExchange;
    if(goldExchange.find(key) == goldExchange.end())
    {
        return;
    }

    if(client->enoughMoney(goldExchange[key].diamond))
    {
        client->addMoney(goldExchange[key].diamond * -1);
        if(goldExchange[key].money != 0)
            client->addMoney1(goldExchange[key].money);

        client->noticeMessageBox("购买成功");
        res = 1;
    }
    else
    {
        client->noticeMessageBox("购买失败");
        res = 0;
    }

	PROTO_VAR_PUBLIC(S_Exchange, snd);
	snd.set_result(res);
	sendToClient(ccid, sndCode, snd);
}

//void ClientManager::proto_C_G13_GiveUp(const ProtoMsgPtr& proto, ClientConnectionId ccid)
//{
//    auto client = ClientManager::me().getByCcid(ccid);
//    if (client == nullptr)
//        return;
//
//    if (client->roomid() == 0)
//    {
//        client->noticeMessageBox("已经不在房间内了");
//
//        //端有时会卡着没退出去, 多给它发个消息
//        PROTO_VAR_PUBLIC(S_G13_PlayerQuited, snd);
//        client->sendToMe(sndCode, snd);
//        return;
//    }
//
//    auto roomPtr = Room::get(client->roomid());
//    if(roomPtr != nullptr)
//    {
//        roomPtr->giveUp(client);
//    }
//}

//void ClientManager::proto_C_G13_ReadyFlag(const ProtoMsgPtr& proto, ClientConnectionId ccid)
//{
//    auto client = ClientManager::me().getByCcid(ccid);
//    if (client == nullptr)
//        return;
//
//    if (client->roomid() == 0)
//    {
//        client->noticeMessageBox("已经不在房间内了");
//
//        //端有时会卡着没退出去, 多给它发个消息
//        PROTO_VAR_PUBLIC(S_G13_PlayerQuited, snd);
//        client->sendToMe(sndCode, snd);
//        return;
//    }
//
//    auto roomPtr = Room::get(client->roomid());
//    if(roomPtr != nullptr)
//    {
//        roomPtr->readyFlag(proto, client);
//    }
//}

//void ClientManager::proto_C_G13_BringOut(const ProtoMsgPtr& proto, ClientConnectionId ccid)
//{
//    auto client = ClientManager::me().getByCcid(ccid);
//    if (client == nullptr)
//        return;
//
//    if (client->roomid() == 0)
//    {
//        client->noticeMessageBox("已经不在房间内了");
//
//        //端有时会卡着没退出去, 多给它发个消息
//        PROTO_VAR_PUBLIC(S_G13_PlayerQuited, snd);
//        client->sendToMe(sndCode, snd);
//        return;
//    }
//
//    auto roomPtr = Room::get(client->roomid());
//    if(roomPtr != nullptr)
//    {
//        roomPtr->bringOut(proto, client);
//    }
//}

void ClientManager::regMsgHandler()
{
    using namespace std::placeholders;
    /************msg from client***********/
    REG_PROTO_PUBLIC(C_SendChat, std::bind(&ClientManager::proto_C_SendChat, this, _1, _2));
    REG_PROTO_PUBLIC(C_G13_ReqGameHistoryCount, std::bind(&ClientManager::proto_C_G13_ReqGameHistoryCount, this, _2));
    REG_PROTO_PUBLIC(C_G13_ReqGameHistoryDetial, std::bind(&ClientManager::proto_C_G13_ReqGameHistoryDetial, this, _1, _2));
    REG_PROTO_PUBLIC(C_G13_OnShareAdvByWeChat, std::bind(&ClientManager::proto_C_G13_OnShareAdvByWeChat, this, _2));
    REG_PROTO_PUBLIC(C_G13_Payment, std::bind(&ClientManager::proto_C_G13_Payment, this, _1, _2));
    REG_PROTO_PUBLIC(C_Exchange, std::bind(&ClientManager::proto_C_Exchange, this, _1, _2));
    //房间通用协议
    /************msg from cluster**********/
    REG_PROTO_PRIVATE(LoginQuest, std::bind(&ClientManager::proto_LoginQuest, this, _1, _2));
    REG_PROTO_PRIVATE(ClientDisconnected, std::bind(&ClientManager::proto_ClientDisconnected, this, _1, _2));
}

}

