#include "water13.h"
#include "client.h"

#include "game_config.h"

#include "dbadaptcher/redis_handler.h"

#include "componet/logger.h"
#include "componet/scope_guard.h"
#include "componet/tools.h"
#include "componet/format.h"
#include "componet/random.h"

#include "protocol/protobuf/public/client.codedef.h"

#include "permanent/proto/water13.pb.h"

#include <random>
#include <algorithm>
#include "water13_card.h"
#include "water13_gold.h"

namespace lobby{

Deck Water13::s_deck;

bool Water13::recoveryFromDB()
{
    using water::dbadaptcher::RedisHandler;
    RedisHandler& redis = RedisHandler::me();
    bool successed = true;
    auto exec = [&successed](const std::string roomid, const std::string& bin) -> bool
    {
        auto game = Water13::deserialize(bin);
        if (game == nullptr)
        {
            LOG_ERROR("Water13, recoveryFromDB, deserialize failed, roomid={}", roomid);
            successed = false;
            return false;
        }
        if (Room::add(game) == false)
        {
            LOG_ERROR("Water13, recoveryFromDB, room::add failed, roomid={}", roomid);
            successed = false;
            return false;
        }
        LOG_TRACE("Water13, laodFromDB, roomid={}, cuid={}", roomid, game->ownerCuid());
        return true;
    };

    //TODO 金币场,要将房间存入list or 金币场不存储房间

    redis.htraversal(ROOM_TABLE_NAME, exec);
    LOG_TRACE("Water13, recoveryFromDB, ok");
    return true;
}

bool Water13::saveToDB(Water13::Ptr game, const std::string& log, ClientPtr client)
{
    //TODO 房卡场 与 金币场 是否要分开处理存储,金币场会存储一些不必要的冗余数据
    if (game == nullptr)
        return false;

    const std::string& bin = serialize(game);
    if (bin.size() == 0)
    {
        LOG_ERROR("Water13, save to DB, {}, serialize failed, roomid={}, cuid={}, opneid={}",
                log, game->getId(), client->cuid(), client->openid());
        return false;
    }

    using water::dbadaptcher::RedisHandler;
    RedisHandler& redis = RedisHandler::me();
    if (!redis.hset(ROOM_TABLE_NAME, componet::format("{}", game->getId()), bin))
    {
        LOG_ERROR("Water13, save to DB, {}, hset failed, roomid={}, cuid={}, opneid={}",
                log, game->getId(), client->cuid(), client->openid());
        return false;
    }

    LOG_TRACE("Water13, save to DB, {}, successed, roomid={}, cuid={}, opneid={}",
            log, game->getId(), client->cuid(), client->openid());
    return true;
}

Water13::Ptr Water13::deserialize(const std::string& bin)
{
    // TODO 同上,若分开存储,则分开读取
    permanent::GameRoom proto;
    if (!proto.ParseFromString(bin))
        return nullptr;

    if (proto.type() != underlying(GameType::water13) && proto.type() != underlying(GameType::water13_gold))
        return nullptr;

    std::shared_ptr<Water13> obj = nullptr;
    if(proto.type() == underlying(GameType::water13))
        obj = Water13_Card::create(proto.roomid(), proto.owner_cuid(), GameType::water13);
    else
        obj = Water13_Gold::create(proto.roomid(), proto.owner_cuid(), GameType::water13_gold);

    //游戏相关的基本数据
    obj->m_status           = static_cast<GameStatus>(proto.g13data().status());
    obj->m_rounds           = proto.g13data().rounds();
    obj->m_startVoteTime    = proto.g13data().start_vote_time();
    obj->m_voteSponsorCuid  = proto.g13data().vote_sponsor_cuid();
    obj->m_settleAllTime    = proto.g13data().settle_all_time();
    obj->m_quickStartTime   = proto.g13data().quick_start_time();
    obj->m_selBankerTime    = proto.g13data().sel_banker_time();
    obj->m_bankerCuid       = proto.g13data().banker_cuid();
    obj->m_selMultipleTime  = proto.g13data().sel_multiple_time();
    obj->m_settleTime       = proto.g13data().settle_time();
    obj->m_newOwnerId       = proto.g13data().new_owner_id();
    if(proto.g13data().remain_brands().size() > 0)
    {
        obj->remainBrands.resize(proto.g13data().remain_brands().size());
        std::copy(proto.g13data().remain_brands().begin(), proto.g13data().remain_brands().end(), obj->remainBrands.begin());
    }

    //房间固定属性
    const auto& attr = proto.g13data().attr();
    obj->m_attr.playType   = attr.play_type();
    obj->m_attr.payor      = attr.payor();
    obj->m_attr.daQiang    = attr.daqiang();
    obj->m_attr.rounds     = attr.rounds();
    obj->m_attr.playerSize = attr.size();
    obj->m_attr.maPai      = attr.ma_pai();
    obj->m_attr.isCrazy    = attr.is_crazy();
    obj->m_attr.halfEnter  = attr.half_enter();
    obj->m_attr.withGhost  = attr.with_ghost();
    obj->m_attr.bankerMultiple  = attr.banker_multiple();
    obj->m_attr.playerPrice= attr.price();
    if(attr.suits_color().size() > 0)
    {
        obj->m_attr.suitsColor.resize(attr.suits_color().size());
        std::copy(attr.suits_color().begin(), attr.suits_color().end(), obj->m_attr.suitsColor.begin());
    }
    obj->m_attr.goldPlace  = attr.gold_place();
    obj->m_attr.placeType  = attr.place_type();

    //玩家列表
    const auto& players = proto.g13data().players();
    if (obj->m_attr.playerSize != players.size())       //TODO 是否连空值也存储了
        return nullptr;
    obj->m_players.resize(attr.size());
    for (auto i = 0; i < obj->m_attr.playerSize; ++i)
    {
        obj->m_players[i].cuid   = players[i].cuid();
        obj->m_players[i].name   = players[i].name();
        obj->m_players[i].imgurl = players[i].imgurl();
        obj->m_players[i].status = players[i].status();
        obj->m_players[i].vote   = players[i].vote();
        obj->m_players[i].rank   = players[i].rank();
        obj->m_players[i].cardsSpecBrand = players[i].cards_spec_brand();
        if (players[i].cards().size() != 8 && players[i].cards().size() != 13)
            return nullptr;
        std::copy(players[i].cards().begin(), players[i].cards().end(), obj->m_players[i].cards.begin());
        obj->m_players[i].chipIn = players[i].chipin();
        obj->m_players[i].multiple = players[i].multiple();
        obj->m_players[i].renewReply = players[i].renew_reply();
    }

    //结算记录
    const auto& settles = proto.g13data().settles();
    obj->m_settleData.resize(settles.size()); //注意, 这里得到一串空指针
    auto settle = RoundSettleData::create();
    for (auto i = 0; i < settles.size(); ++i)
    {
        obj->m_settleData[i] = RoundSettleData::create();

        const auto& protoPlayers = settles[i].players();
        if (protoPlayers.size() != obj->m_attr.playerSize)
            return nullptr;

        auto& objPlayers = obj->m_settleData[i]->players;
        objPlayers.resize(obj->m_attr.playerSize);
        for (auto j = 0; j < obj->m_attr.playerSize; ++j)
        {
            objPlayers[j].cuid = protoPlayers[j].cuid();

            if (protoPlayers[j].cards().size() != 13)
                return nullptr;
            std::copy(protoPlayers[j].cards().begin(), protoPlayers[j].cards().end(), objPlayers[j].cards.begin());

            objPlayers[j].dun[0].b = static_cast<Deck::Brand>(protoPlayers[j].dun0().first());
            objPlayers[j].dun[1].b = static_cast<Deck::Brand>(protoPlayers[j].dun1().first());
            objPlayers[j].dun[2].b = static_cast<Deck::Brand>(protoPlayers[j].dun2().first());
            objPlayers[j].dun[0].point = protoPlayers[j].dun0().second();
            objPlayers[j].dun[1].point = protoPlayers[j].dun1().second();
            objPlayers[j].dun[2].point = protoPlayers[j].dun2().second();

            objPlayers[j].spec  = static_cast<Deck::G13SpecialBrand>(protoPlayers[j].spec());
            objPlayers[j].prize = protoPlayers[j].prize();

            for (const auto& loser : protoPlayers[j].losers())
            {
                objPlayers[j].losers[loser.k()][0] = loser.f0();
                objPlayers[j].losers[loser.k()][1] = loser.f1();
            }

            objPlayers[j].quanLeiDa = protoPlayers[j].quanleida();
            objPlayers[j].hasMaPai = protoPlayers[j].hasmapai();
        }

    }
    return obj;
}

std::string Water13::serialize(Water13::Ptr obj)
{
    return obj->serialize();
}

std::string Water13::serialize()
{
    bool isCardMode = typeid(*this) == typeid(Water13_Card);

    permanent::GameRoom proto;
    proto.set_roomid(getId());
    if(isCardMode)
        proto.set_type(underlying(GameType::water13));
    else
        proto.set_type(underlying(GameType::water13_gold));
    proto.set_owner_cuid(ownerCuid());
    
    //游戏房间基本数据
    auto gameData = proto.mutable_g13data();
    gameData->set_status(underlying(m_status));
    gameData->set_rounds(m_rounds);
    gameData->set_start_vote_time(m_startVoteTime);
    gameData->set_vote_sponsor_cuid(m_voteSponsorCuid);
    gameData->set_settle_all_time(m_settleAllTime);
    gameData->set_quick_start_time(m_quickStartTime);
    gameData->set_sel_banker_time(m_selBankerTime);
    gameData->set_banker_cuid(m_bankerCuid);
    gameData->set_sel_multiple_time(m_selMultipleTime);
    gameData->set_settle_time(m_settleTime);
    gameData->set_new_owner_id(m_newOwnerId);
    for (auto brands : remainBrands)
        gameData->add_remain_brands(brands);

    //房间固定属性
    auto attrData = gameData->mutable_attr();
    attrData->set_play_type(m_attr.playType);
    attrData->set_payor(m_attr.payor);
    attrData->set_daqiang(m_attr.daQiang);
    attrData->set_rounds(m_attr.rounds);
    attrData->set_size(m_attr.playerSize);
    attrData->set_ma_pai(m_attr.maPai);
    attrData->set_is_crazy(m_attr.isCrazy);
    attrData->set_half_enter(m_attr.halfEnter);
    attrData->set_with_ghost(m_attr.withGhost);
    attrData->set_banker_multiple(m_attr.bankerMultiple);
    attrData->set_price(m_attr.playerPrice);
    for (auto suit : m_attr.suitsColor)
        attrData->add_suits_color(suit);
    attrData->set_gold_place(m_attr.goldPlace);
    attrData->set_place_type(m_attr.placeType);

    //玩家列表
    for (const auto& playerInfo : m_players)
    {
        auto playerData = gameData->add_players();
        playerData->set_cuid  (playerInfo.cuid);
        playerData->set_name  (playerInfo.name);
        playerData->set_imgurl(playerInfo.imgurl);
        playerData->set_status(playerInfo.status);
        playerData->set_vote  (playerInfo.vote);
        playerData->set_rank  (playerInfo.rank);
        playerData->set_cards_spec_brand(playerInfo.cardsSpecBrand);
        for (auto card : playerInfo.cards)
            playerData->add_cards(card);
        playerData->set_chipin(playerInfo.chipIn);
        playerData->set_multiple(playerInfo.multiple);
        playerData->set_renew_reply(playerInfo.renewReply);
    }

    //结算记录
    for (const auto& settle : m_settleData)
    {
        auto settleData = gameData->add_settles();
        for (const auto& player : settle->players)
        {
            auto playerData = settleData->add_players();
            playerData->set_cuid(player.cuid);
            for (auto card : player.cards)
                playerData->add_cards(card);
            playerData->mutable_dun0()->set_first(underlying(player.dun[0].b));
            playerData->mutable_dun0()->set_second(player.dun[0].point);
            playerData->mutable_dun0()->set_third(player.dun_prize[0]);
            playerData->mutable_dun1()->set_first(underlying(player.dun[1].b));
            playerData->mutable_dun1()->set_second(player.dun[1].point);
            playerData->mutable_dun1()->set_third(player.dun_prize[1]);
            playerData->mutable_dun2()->set_first(underlying(player.dun[2].b));
            playerData->mutable_dun2()->set_second(player.dun[2].point);
            playerData->mutable_dun2()->set_third(player.dun_prize[2]);
            playerData->set_prize(player.prize);
            for (const auto& item : player.losers)
            {
                auto loserData = playerData->add_losers();
                loserData->set_k(item.first);
                loserData->set_f0(item.second[0]);
                loserData->set_f1(item.second[1]);
            }
            playerData->set_quanleida(player.quanLeiDa);
            playerData->set_hasmapai(player.hasMaPai);
        }
    }

    std::string ret;
    return proto.SerializeToString(&ret) ? ret : "";
}

Water13::Ptr Water13::getByRoomId(RoomId roomid)
{
    //TODO 如果存储位置类型值不对,子类要重写该函数
    return std::static_pointer_cast<Water13>(Room::get(roomid));
}

//TODO 子类通用的协议,在这里注册
void Water13::regMsgHandler()
{
    using namespace std::placeholders;
    REG_PROTO_PUBLIC(C_G13_GiveUp, std::bind(&Water13::proto_C_G13_GiveUp, _1, _2));
    REG_PROTO_PUBLIC(C_G13_ReadyFlag, std::bind(&Water13::proto_C_G13_ReadyFlag, _1, _2));
    REG_PROTO_PUBLIC(C_G13_BringOut, std::bind(&Water13::proto_C_G13_BringOut, _1, _2));
    REG_PROTO_PUBLIC(C_G13_SelectMultiple, std::bind(&Water13::proto_C_G13_SelectMultiple, _1, _2));
    REG_PROTO_PUBLIC(C_G13_ChipInPoint, std::bind(&Water13::proto_C_G13_ChipInPoint, _1, _2));
}

void Water13::proto_C_G13_GiveUp(ProtoMsgPtr proto, ClientConnectionId ccid)
{
    auto client = ClientManager::me().getByCcid(ccid);
    if (client == nullptr)
        return;

    if (client->roomid() == 0)
    {
        client->noticeMessageBox("已经不在房间内了");

        //端有时会卡着没退出去, 多给它发个消息
        PROTO_VAR_PUBLIC(S_G13_PlayerQuited, snd);
        client->sendToMe(sndCode, snd);
        return;
    }

    auto roomPtr = Room::get(client->roomid());
    if(roomPtr != nullptr)
    {
        roomPtr->giveUp(client);
    }
}

void Water13::proto_C_G13_ReadyFlag(ProtoMsgPtr proto, ClientConnectionId ccid)
{
    auto client = ClientManager::me().getByCcid(ccid);
    if (client == nullptr)
        return;

    if (client->roomid() == 0)
    {
        client->noticeMessageBox("已经不在房间内了");

        //端有时会卡着没退出去, 多给它发个消息
        PROTO_VAR_PUBLIC(S_G13_PlayerQuited, snd);
        client->sendToMe(sndCode, snd);
        return;
    }

    auto roomPtr = Room::get(client->roomid());
    if(roomPtr != nullptr)
    {
        roomPtr->readyFlag(proto, client);
    }
}

void Water13::proto_C_G13_BringOut(ProtoMsgPtr proto, ClientConnectionId ccid)
{
    auto client = ClientManager::me().getByCcid(ccid);
    if (client == nullptr)
        return;

    if (client->roomid() == 0)
    {
        client->noticeMessageBox("已经不在房间内了");

        //端有时会卡着没退出去, 多给它发个消息
        PROTO_VAR_PUBLIC(S_G13_PlayerQuited, snd);
        client->sendToMe(sndCode, snd);
        return;
    }

    auto roomPtr = Room::get(client->roomid());
    if(roomPtr != nullptr)
    {
        roomPtr->bringOut(proto, client);
    }
}


//void Water13::proto_C_G13_SimulationRound(ProtoMsgPtr proto, ClientConnectionId ccid)
//{
//    auto client = ClientManager::me().getByCcid(ccid);
//    if (client == nullptr)
//        return;
//
//    auto rcv = PROTO_PTR_CAST_PUBLIC(C_G13_SimulationRound, proto);
//    if (rcv->players().size() < 2)
//    {
//        client->noticeMessageBox("出牌人数过少");
//        LOG_TRACE("模拟局, 房间人数不对, 收到{}", rcv->players().size());
//        return;
//    }
//    struct ItemInfo
//    {
//        bool cardsSpecBrand = false;
//        std::array<Deck::Card, 13> cards;
//        ClientUniqueId cuid;
//    };
//
//    std::vector<ItemInfo> playerList;
//    std::map<ClientUniqueId, std::array<Deck::Card, 13>> cuid2Cards; //多费了内存, 就这样吧
//    for(const auto& item : rcv->players())
//    {
//        if(item.cards_size() != 13)
//        {
//            client->noticeMessageBox("出牌数量不对");
//            LOG_TRACE("模拟局, 出牌数量不对, {}", item.cards_size());
//        }
//        playerList.emplace_back();
//        playerList.back().cardsSpecBrand = item.special();
//        for (auto i = 0u; i < playerList.back().cards.size(); ++i) //这里很蛋疼, 端的数据不保证正确
//        {
//            const uint32_t itemCardsSize = item.cards().size();
//            playerList.back().cards[i] = i < itemCardsSize ? item.cards()[i] : 1;
//            if (playerList.back().cards[i] < 1 || playerList.back().cards[i] > 52)
//                playerList.back().cards[i] = 1;
//        }
//        playerList.back().cuid = item.cuid();
//        cuid2Cards[item.cuid()] = playerList.back().cards;
//    }
//
//    auto result = calcRound(playerList, DQ_NORMAL, 0);
//    PROTO_VAR_PUBLIC(S_G13_CalcRoundSimulationRet, snd)
//    for (const auto& pd : result->players)
//    {
//        auto player = snd.mutable_result()->add_players();
//        player->set_cuid(pd.cuid);
//        for (Deck::Card crd : cuid2Cards[pd.cuid])
//            player->add_cards(crd);
//        player->set_rank(pd.prize);
//        player->mutable_dun0()->set_brand(static_cast<int32_t>(pd.dun[0].b));
//        player->mutable_dun0()->set_point(static_cast<int32_t>(pd.dun[0].point));
//        player->mutable_dun1()->set_brand(static_cast<int32_t>(pd.dun[1].b));
//        player->mutable_dun1()->set_point(static_cast<int32_t>(pd.dun[1].point));
//        player->mutable_dun2()->set_brand(static_cast<int32_t>(pd.dun[2].b));
//        player->mutable_dun2()->set_point(static_cast<int32_t>(pd.dun[2].point));
//        player->mutable_spec()->set_brand(static_cast<int32_t>(pd.spec));
//        player->mutable_spec()->set_point(static_cast<int32_t>(0));
//    }
//    client->sendToMe(sndCode, snd);
//}


/*************************************************************************/
bool Water13::enterRoom(Client::Ptr client)
{
    if (client == nullptr)
        return false;

    if (client->roomid() != 0)
    {
        if (client->roomid() == getId()) //就在这个房中, 直接忽略
            return true;

        client->noticeMessageBox("已经在{}号房间中了!", client->roomid());
        return false;
    }

    bool isCardMode = typeid(*this) == typeid(Water13_Card);

    if(!(isCardMode && m_attr.halfEnter))
    {
        if (m_status != GameStatus::prepare || m_rounds != 0)
        {
            client->noticeMessageBox("进入失败, 此房间的游戏已经开始");
            return false;
        }
    }

    const uint32_t index = getEmptySeatIndex();
    if (index == NO_POS)
    {
        client->noticeMessageBox("房间已满, 无法加入");
        return false;
    }

    if(isCardMode)
    {
        //钱数检查
        bool enoughMoney = false;
        const int32_t totalPrice = m_attr.playerPrice * m_attr.playerSize;
        switch (m_attr.payor)
        {
            case PAY_BANKER:
                enoughMoney = (ownerCuid() == client->cuid()) ?  client->enoughMoney(totalPrice) : true;
                break;
            case PAY_AA:
                enoughMoney = client->enoughMoney(m_attr.playerPrice);
                break;
            case PAY_WINNER:
                enoughMoney = client->enoughMoney(totalPrice);
                break;
            default:
                return false;
        }
        if (!enoughMoney)
        {
            (ownerCuid() == client->cuid()) ? client->noticeMessageBox("创建失败, 钻石不足") : client->noticeMessageBox("进入失败, 钻石不足");
            return false;
        }
    }

    //加入成员列表
    m_players[index].cuid = client->cuid();
    m_players[index].lastCuid = client->cuid();
    m_players[index].name = client->name();
    m_players[index].imgurl = client->imgurl();
    m_players[index].ipstr = client->ipstr();
    m_players[index].status = (m_rounds == 0 && m_status == GameStatus::prepare) ? PublicProto::S_G13_PlayersInRoom::PREP : PublicProto::S_G13_PlayersInRoom::WITNESS;

    //反向索引, 记录房间号
    client->setRoomId(getId());

    LOG_TRACE("G13, {}房间成功, roomid={}, name={}, ccid={}, cuid={}, openid={}", (ownerCuid() != client->cuid()) ? "进入" : "创建",
            client->roomid(), client->name(), client->ccid(), client->cuid(), client->openid());

    afterEnterRoom(client);
    return true;
}

void Water13::clientOnlineExec(Client::Ptr client)
{
    if (client == nullptr)
        return;
    if (client->roomid() != getId())
        return;
    auto info = getPlayerInfoByCuid(client->cuid());
    if (info == nullptr)
    {
        LOG_TRACE("玩家上线, 房间号已被复用, roomid={}, name={}, ccid={}, cuid={}, openid={}", getId(), client->name(), client->ccid(), client->cuid(), client->openid());
        client->setRoomId(0);
        return;
    }

    LOG_TRACE("client online, sync gameinfo, roomid={}, name={}, ccid={}, cuid={}, openid={}", getId(), client->name(), client->ccid(), client->cuid(), client->openid());
    info->name   = client->name();
    info->imgurl = client->imgurl();
    info->ipstr  = client->ipstr();
    info->online = true;

    afterEnterRoom(client);
}

void Water13::clientOfflineExec(ClientPtr client)
{
    if (client == nullptr)
        return;

    if (client->roomid() != getId())
        return;
    
    auto info = getPlayerInfoByCuid(client->cuid());
    if (info == nullptr)
    {
        LOG_ERROR("玩家下线, 房间号已失效, roomid={}, name={}, ccid={}, cuid={}, openid={}", getId(), client->name(), client->ccid(), client->cuid(), client->openid());
        client->setRoomId(0);
        return;
    }

    info->online = false;

    LOG_TRACE("client offline, sync gameinfo, roomid={}, name={}, ccid={}, cuid={}, openid={}", getId(), client->name(), client->ccid(), client->cuid(), client->openid());
    syncAllPlayersInfoToAllClients();
}

void Water13::afterEnterRoom(ClientPtr client)
{
    if (client == nullptr)
        return;

    {//给进入者发送房间基本属性
        PROTO_VAR_PUBLIC(S_G13_RoomAttr, snd)
        snd.set_room_id(getId());
        snd.set_banker_cuid(ownerCuid());
        snd.mutable_attr()->set_play_type(m_attr.playType);
        snd.mutable_attr()->set_payor(m_attr.payor);
        snd.mutable_attr()->set_da_qiang(m_attr.daQiang);
        snd.mutable_attr()->set_rounds(m_attr.rounds);
        snd.mutable_attr()->set_player_size(m_attr.playerSize);
        snd.mutable_attr()->set_ma_pai(m_attr.maPai);
        snd.mutable_attr()->set_is_crazy(m_attr.isCrazy);
        snd.mutable_attr()->set_half_enter(m_attr.halfEnter);
        snd.mutable_attr()->set_with_ghost(m_attr.withGhost);
        snd.mutable_attr()->set_banker_multiple(m_attr.bankerMultiple);
        auto begin = m_attr.suitsColor.begin();
        for( ; begin != m_attr.suitsColor.end(); ++begin)
        {
            snd.mutable_attr()->add_suits_colors(*begin);
        }
        snd.mutable_attr()->set_gold_place(m_attr.goldPlace);
        snd.mutable_attr()->set_place_type(m_attr.placeType);
        client->sendToMe(sndCode, snd);
    }

    //给所有人发送成员列表
    syncAllPlayersInfoToAllClients();

    //投票退出协议
    PROTO_VAR_PUBLIC(S_G13_AbortGameOrNot, snd3);
    snd3.set_sponsor(m_voteSponsorCuid);
    time_t elapse = componet::toUnixTime(s_timerTime) - m_startVoteTime;
    snd3.set_remain_seconds(MAX_VOTE_DURATION > elapse ? MAX_VOTE_DURATION - elapse : 0);

    //提前开始协议
    PROTO_VAR_PUBLIC(S_G13_QuickStartOrNot, snd4);
    elapse = componet::toUnixTime(s_timerTime) - m_quickStartTime;
    snd4.set_remain_seconds(MAX_VOTE_QUICK_START_DURATION > elapse ? MAX_VOTE_QUICK_START_DURATION - elapse : 0);

    //抢庄协议
    PROTO_VAR_PUBLIC(S_G13_ChipInInfo, snd5);
    elapse = componet::toUnixTime(s_timerTime) - m_selBankerTime;
    snd5.set_remain_seconds(MAX_SELECT_BANKER_DURATION > elapse ? MAX_SELECT_BANKER_DURATION - elapse : 0);

    //闲家点数协议
    PROTO_VAR_PUBLIC(S_G13_MultipleInfo, snd6);
    elapse = componet::toUnixTime(s_timerTime) - m_selMultipleTime;
    snd6.set_remain_seconds(MAX_SELECT_MULTIPLE_DURATION > elapse ? MAX_SELECT_MULTIPLE_DURATION - elapse : 0);


    //给进入者发送他自己的牌信息
    if (m_status == GameStatus::play || m_status == GameStatus::vote || m_status == GameStatus::quickStart || m_status == GameStatus::selBanker || m_status == GameStatus::selMultiple)
    {
        for (const PlayerInfo& info : m_players)
        {
            //同步手牌到端
            if (info.cuid == client->cuid())
            {
                if(m_status == GameStatus::play || m_status == GameStatus::vote || m_status == GameStatus::selBanker || m_status == GameStatus::selMultiple)
                {
                    PROTO_VAR_PUBLIC(S_G13_HandOfMine, snd2)
                        snd2.set_rounds(m_rounds);
                    for (auto card : info.cards)
                        snd2.add_cards(card);
                    client->sendToMe(snd2Code, snd2);
                }
            }

            auto voteInfo = snd3.add_votes();
            voteInfo->set_cuid(info.cuid);
            voteInfo->set_vote(info.vote);

            auto qsVote = snd4.add_votes();
            qsVote->set_cuid(info.cuid);
            qsVote->set_vote(info.vote);

            auto bankerInfo = snd5.add_list();
            bankerInfo->set_cuid(info.cuid);
            bankerInfo->set_multiple(info.chipIn);

            if(info.cuid != m_bankerCuid)
            {
                auto miVote = snd6.add_list();
                miVote->set_cuid(info.cuid);
                miVote->set_multiple(info.multiple);
            }
        }
    }
    if (m_status == GameStatus::vote)
    {
        client->sendToMe(snd3Code, snd3);
    }

    if(m_status == GameStatus::quickStart)
    {
        client->sendToMe(snd4Code, snd4);
    }

    if(m_status == GameStatus::selBanker)
    {
        client->sendToMe(snd5Code, snd5);
    }

    if(m_attr.playType == GP_BANKER && (m_status == GameStatus::selMultiple || m_status == GameStatus::play))
    {
        client->sendToMe(snd6Code, snd6);
    }

    if(m_status == GameStatus::settle)
    {
        auto settleEndData  = m_settleData.rbegin();
        if(settleEndData != m_settleData.rend())
        {
            auto curRound = *settleEndData;
            //发结果
            PROTO_VAR_PUBLIC(S_G13_AllHands, snd7)
                for (const auto& pd : curRound->players)
                {
                    auto player = snd7.add_players();
                    player->set_cuid(pd.cuid);
                    for (Deck::Card crd : pd.cards)
                        player->add_cards(crd);
                    player->set_rank(pd.prize);
                    player->mutable_dun0()->set_brand(static_cast<int32_t>(pd.dun[0].b));
                    player->mutable_dun0()->set_point(static_cast<int32_t>(pd.dun[0].point));
                    player->mutable_dun0()->set_score(static_cast<int32_t>(pd.dun_prize[0]));
                    player->mutable_dun1()->set_brand(static_cast<int32_t>(pd.dun[1].b));
                    player->mutable_dun1()->set_point(static_cast<int32_t>(pd.dun[1].point));
                    player->mutable_dun1()->set_score(static_cast<int32_t>(pd.dun_prize[1]));
                    player->mutable_dun2()->set_brand(static_cast<int32_t>(pd.dun[2].b));
                    player->mutable_dun2()->set_point(static_cast<int32_t>(pd.dun[2].point));
                    player->mutable_dun2()->set_score(static_cast<int32_t>(pd.dun_prize[2]));
                    player->mutable_spec()->set_brand(static_cast<int32_t>(pd.spec));
                    player->mutable_spec()->set_point(static_cast<int32_t>(0));
                    player->mutable_spec()->set_score(static_cast<int32_t>(0));
                }
            sendToAll(snd7Code, snd7);
        }
    }

    //广播庄家信息
    if(m_bankerCuid > 0)
    {
        PROTO_VAR_PUBLIC(S_G13_BankerInfo, snd8);
        snd8.set_cuid(m_bankerCuid);
        sendToAll(snd8Code, snd8);
    }
}

void Water13::sendToAll(TcpMsgCode msgCode, const ProtoMsg& proto)
{
    for(const PlayerInfo& info : m_players)
    {
        if (info.cuid == 0)
            continue;

        Client::Ptr client = ClientManager::me().getByCuid(info.cuid);
        if (client != nullptr)
            client->sendToMe(msgCode, proto);
    }
}

void Water13::sendToOthers(ClientUniqueId cuid, TcpMsgCode msgCode, const ProtoMsg& proto)
{
    for(const PlayerInfo& info : m_players)
    {
        if (info.cuid == 0 || info.cuid == cuid)
            continue;

        Client::Ptr client = ClientManager::me().getByCuid(info.cuid);
        if (client != nullptr)
            client->sendToMe(msgCode, proto);
    }
}

void Water13::syncAllPlayersInfoToAllClients()
{
    PROTO_VAR_PUBLIC(S_G13_PlayersInRoom, snd)
    for(const PlayerInfo& info : m_players)
    {
        using namespace PublicProto;
        auto player = snd.add_players();
        player->set_cuid(info.cuid);
        player->set_name(info.name);
        player->set_imgurl(info.imgurl);
        player->set_ipstr(info.ipstr);
        player->set_status(info.status);
        player->set_rank(info.rank);
        player->set_online(info.online);
    }
    snd.set_rounds(m_rounds);
    sendToAll(sndCode, snd);
}

void Water13::removePlayer(ClientPtr client)
{
    if (client == nullptr)
        return;

    for (auto iter = m_players.begin(); iter != m_players.end(); ++iter)
    {
        if (iter->cuid == client->cuid())
        {
            G13His::Detail::Ptr settleHisDetail = nullptr;
            if (m_status == GameStatus::settleAll)
            {
                auto& info = *iter;
                settleHisDetail = G13His::Detail::create();
                settleHisDetail->roomid = getId();
                settleHisDetail->rank   = info.rank;
                settleHisDetail->time   = componet::toUnixTime(s_timerTime);
                settleHisDetail->opps.reserve(m_players.size());
                for (const PlayerInfo& oppInfo : m_players)
                {
                    if (oppInfo.cuid == info.cuid)
                        continue;
                    settleHisDetail->opps.resize(settleHisDetail->opps.size() + 1);
                    settleHisDetail->opps.back().cuid = oppInfo.lastCuid;
                    settleHisDetail->opps.back().name = oppInfo.name;
                    settleHisDetail->opps.back().rank = oppInfo.rank;
                }
            }
            client->afterLeaveRoom(settleHisDetail);
            iter->clear();
            LOG_TRACE("Water13, reomvePlayer, roomid={}, name={}, ccid={}, cuid={}, openid={}",
                        getId(), client->name(), client->ccid(), client->cuid(), client->openid());
            break;
        }
    }
    if (m_status != GameStatus::settleAll && m_status != GameStatus::closed)
        syncAllPlayersInfoToAllClients();
    return;
}

void Water13::abortGame()
{
    //踢所有人
    for (const PlayerInfo& info : m_players)
    {
        LOG_TRACE("Water13, abortGame, 踢人, roomid={}, cuid={}", getId(), info.cuid);
        Client::Ptr client = ClientManager::me().getByCuid(info.cuid);
        if (client != nullptr)
        {
            G13His::Detail::Ptr settleHisDetail = nullptr;
            if (m_status == GameStatus::settleAll)
            {
                settleHisDetail = G13His::Detail::create();
                settleHisDetail->roomid = getId();
                settleHisDetail->rank   = info.rank;
                settleHisDetail->time   = componet::toUnixTime(s_timerTime);
                settleHisDetail->opps.reserve(m_players.size());
                for (const PlayerInfo& oppInfo : m_players)
                {
                    if (oppInfo.cuid == info.cuid)
                        continue;
                    settleHisDetail->opps.resize(settleHisDetail->opps.size() + 1);
                    settleHisDetail->opps.back().cuid = oppInfo.lastCuid;
                    settleHisDetail->opps.back().name = oppInfo.name;
                    settleHisDetail->opps.back().rank = oppInfo.rank;
                }
            }
            client->afterLeaveRoom(settleHisDetail);
        }
    }
    m_players.clear();
    m_status = GameStatus::closed;
    destroyLater();
}

void Water13::abortGame(const std::string& noticeToClient)
{
    //踢所有人
    for (const PlayerInfo& info : m_players)
    {
        LOG_TRACE("Water13, abortGame, 踢人, roomid={}, cuid={}", getId(), info.cuid);
        Client::Ptr client = ClientManager::me().getByCuid(info.cuid);
        if (client != nullptr)
        {
            G13His::Detail::Ptr settleHisDetail = nullptr;
            if (m_status == GameStatus::settleAll)
            {
                settleHisDetail = G13His::Detail::create();
                settleHisDetail->roomid = getId();
                settleHisDetail->rank   = info.rank;
                settleHisDetail->time   = componet::toUnixTime(s_timerTime);
                settleHisDetail->opps.reserve(m_players.size());
                for (const PlayerInfo& oppInfo : m_players)
                {
                    if (oppInfo.cuid == info.cuid)
                        continue;
                    settleHisDetail->opps.resize(settleHisDetail->opps.size() + 1);
                    settleHisDetail->opps.back().cuid = oppInfo.lastCuid;
                    settleHisDetail->opps.back().name = oppInfo.name;
                    settleHisDetail->opps.back().rank = oppInfo.rank;
                }
            }
            client->afterLeaveRoom(settleHisDetail);

        if (noticeToClient != "")
            client->noticeMessageBox(noticeToClient);
        }
    }
    m_players.clear();
    m_status = GameStatus::closed;
    destroyLater();
}

void Water13::tryStartRound()
{
    //everyone ready
    if (m_status == GameStatus::prepare)
    {
        for (const PlayerInfo& info : m_players)
        {
            if (info.cuid == 0 || info.status != PublicProto::S_G13_PlayersInRoom::READY)
                return;
            if (ClientManager::me().getByCuid(info.cuid) == nullptr) //必须都在线, 因为第一局要扣钱
                return;
        }
    }
    else if(m_status == GameStatus::quickStart)
    {
        //quickStart状态中,后进入玩家都自动准备
    }
    else
    {
        if(m_status == GameStatus::settle)
        {
            for (const PlayerInfo* info : getValidPlayers())
            {
                //TODO 看需不要删除比牌状态的判断
                if (info->status != PublicProto::S_G13_PlayersInRoom::COMPARE && info->status != PublicProto::S_G13_PlayersInRoom::READY)
                {
                    LOG_ERROR("tryStartRound, 房间玩家数据错误, rounds={}/{}, roomid={}, info.cuid={}, info.status={}",
                            m_rounds, m_attr.rounds, getId(), info->cuid, info->status);
                    return;
                }
            }
        }
        else
        {
            return;
        }
    }

    //牌局开始
    ++m_rounds;
    LOG_TRACE("新一轮开始, rounds={}/{}, roomid={}", m_rounds, m_attr.rounds, getId());

    //shuffle
    {
        //初始化牌组
        {
            Water13::s_deck.cards.clear();
            Water13::s_deck.cards.reserve(108);
            int32_t cards_size = 52;
            if(m_attr.withGhost)
                cards_size = 54;

            for (int32_t i = 0; i < cards_size; ++i)
                Water13::s_deck.cards.push_back(i + 1);

            if(m_attr.withGhost && m_attr.playerSize == 8)
            {
                //8人房, 默认4鬼
                Water13::s_deck.cards.push_back(53);
                Water13::s_deck.cards.push_back(54);
            }

            //如果有选花色,按选择花色添加
            if(m_attr.suitsColor.size() > 0)
            {
                auto begin = m_attr.suitsColor.begin();
                for( ; begin != m_attr.suitsColor.end(); ++begin)
                {
                    for(int32_t i = 0; i < 13; ++i)
                    {
                        Water13::s_deck.cards.push_back(*begin * 13 + i + 1);
                    }
                }
            }
            else
            {
                //人数多于4人， 按照 黑红梅方 的优先顺序增加配牌
                const int32_t suitsMap[] = {3, 2, 1, 0};
                for (int32_t p = 5; p <= m_attr.playerSize; ++p)
                {
                    const int32_t suit = suitsMap[(p - 1) % 4];
                    for (int32_t i = 0; i < 13; ++i)
                        Water13::s_deck.cards.push_back(suit * 13 + i + 1);
                }
            }

            Water13::s_deck.cards.shrink_to_fit();
            std::string cardsStr;
            for (uint32_t index = 0; index < Water13::s_deck.cards.size(); ++index)
            {
                cardsStr.append(std::to_string(Water13::s_deck.cards[index]));
                cardsStr.append(",");
            }
            LOG_TRACE("牌组初始化完毕, rounds={}/{}, roomid={}, playerSize={}, [{}]", m_rounds, m_attr.rounds, getId(), m_attr.playerSize, cardsStr);
        }

        static std::random_device rd;
        static std::mt19937 rg(rd());
        std::shuffle(Water13::s_deck.cards.begin(), Water13::s_deck.cards.end(), rg);

        //是否配置了固定发牌(测试用)
        const auto& testDeckCfg = GameConfig::me().data().testDeck;
        if (testDeckCfg.index != -1u)
        {
            if (Water13::s_deck.cards.size() > testDeckCfg.decks.size())
                return;

            for (auto i = 0u; i < Water13::s_deck.cards.size(); ++i)
                Water13::s_deck.cards[i] = testDeckCfg.decks[testDeckCfg.index][i];
            LOG_TRACE("G13, deal deck, fixd hands by config, cfgindex={}", testDeckCfg.index);
        }
    }

    //deal cards, then update player status and send to client
    PROTO_VAR_PUBLIC(S_G13_PlayersInRoom, snd1)
    snd1.set_rounds(m_rounds);
    uint32_t index = 0;
    for (PlayerInfo& info : m_players)
    {
        //后来加入的,没同意前不能开局
        if(info.status == PublicProto::S_G13_PlayersInRoom::WITNESS)
            continue;

        //插入流程,抢庄玩法,先发8张
        uint32_t cardSize =  info.cards.size();
        if(m_attr.playType == GP_BANKER)
        {
            cardSize = 8;
        }

        //发牌
        for (uint32_t i = 0; i < cardSize; ++i)
            info.cards[i] = Water13::s_deck.cards[index++];

        //排个序, 以后检查外挂换牌时好比较
        std::sort(info.cards.begin(), info.cards.end());

        //同步手牌到端
        PROTO_VAR_PUBLIC(S_G13_HandOfMine, snd2)
        snd2.set_rounds(m_rounds);
        std::string cardsStr;
        cardsStr.reserve(42);
        for (auto card : info.cards)
        {
            cardsStr.append(std::to_string(card));
            cardsStr.append(",");
            snd2.add_cards(card);
        }

        auto client = ClientManager::me().getByCuid(info.cuid);
        if (client != nullptr)
        {
            if (m_rounds == 1) //第一局, 要扣钱
            {
                if(m_attr.payor == PAY_BANKER && client->cuid() == ownerCuid())
                {
                    const int32_t price = m_attr.playerPrice * m_attr.playerSize;
                    client->addMoney(-price);
                    LOG_TRACE("游戏开始扣钱, 房主付费, moneyChange={}, roomid={}, name={}, ccid={}, cuid={}, openid={}",
                              -price, getId(), client->name(), client->ccid(), info.cuid, client->openid());
                }
                else if(m_attr.payor == PAY_AA)
                {
                    const int32_t price = m_attr.playerPrice;
                    client->addMoney(-price);
                    LOG_TRACE("游戏开始扣钱, 均摊, moneyChange={}, roomid={}, name={}, ccid={}, cuid={}, openid={}",
                              -price, getId(), client->name(), client->ccid(), info.cuid, client->openid());
                }
            }

            client->sendToMe(snd2Code, snd2);
        }
        else
        {
            if (m_rounds == 1) //函数开头检查过所有乘员都可以获取Client::Ptr成功, 正常情况这里不会被执行才对
                LOG_ERROR("游戏开始, 需要扣费但是玩家不在线, payor={}, roomid={}, cuid={}", m_attr.payor, getId(), info.cuid);
        }

        //玩家状态改变
        if(m_attr.playType == GP_BANKER)
        {
            info.status = PublicProto::S_G13_PlayersInRoom::BANKER;
        }
        else
        {
            info.status = PublicProto::S_G13_PlayersInRoom::SORT;
        }

        auto player = snd1.add_players();
        player->set_status(info.status);
        player->set_cuid(info.cuid);
        player->set_name(info.name);
        player->set_imgurl(info.imgurl);
        player->set_ipstr(info.ipstr);
        player->set_rank(info.rank);
        LOG_TRACE("deal deck, roomid={}, round={}/{}, cuid={}, name={}, cards=[{}]", getId(), m_rounds, m_attr.rounds, info.cuid, info.name, cardsStr);
    }
    sendToAll(snd1Code, snd1);

    //游戏状态改变
    if(m_attr.playType == GP_BANKER)
    {
        m_status = GameStatus::selBanker;
        m_selBankerTime = componet::toUnixTime(s_timerTime);

        //剩余牌组存起来,后面继续发
        uint32_t playerSize = getValidPlayers().size();
        for(uint32_t i = 0; i < playerSize * 5; ++i)
        {
            remainBrands.push_back(Water13::s_deck.cards[index++]);
        }

        PROTO_VAR_PUBLIC(S_G13_ChipInInfo, snd3);
        time_t elapse = componet::toUnixTime(s_timerTime) - m_selBankerTime;
        snd3.set_remain_seconds(MAX_SELECT_BANKER_DURATION > elapse ? MAX_SELECT_BANKER_DURATION - elapse : 0);
        for (PlayerInfo* info : getValidPlayers())
        {
            auto chipInfo = snd3.add_list();
            chipInfo->set_cuid(info->cuid);
            chipInfo->set_multiple(info->chipIn);
        }
        sendToAll(snd3Code, snd3);
    }
    else
    {
        m_status = GameStatus::play;
    }
}

void Water13::trySettleGame()
{
    if (m_status != GameStatus::play)
        return;

    //everyone compare
    for (const PlayerInfo* info : getValidPlayers())
    {
        if (info->status != PublicProto::S_G13_PlayersInRoom::COMPARE ||
                ClientManager::me().getByCuid(info->cuid) == nullptr)
            return;
    }

    //先算牌型
    auto curRound = calcRound(m_players, m_attr.daQiang, m_attr.maPai);
    for (uint32_t i = 0; i < m_players.size(); ++i)
    {
        curRound->players[i].cards = m_players[i].cards;
        m_players[i].rank += curRound->players[i].prize;

        //重置玩家状态
        if(m_players[i].cuid != 0 && m_players[i].status != PublicProto::S_G13_PlayersInRoom::WITNESS)
            m_players[i].status = PublicProto::S_G13_PlayersInRoom::PREP;
    }
    m_settleData.push_back(curRound);

    //发结果
    PROTO_VAR_PUBLIC(S_G13_AllHands, snd)
    for (const auto& pd : curRound->players)
    {
        auto player = snd.add_players();
        player->set_cuid(pd.cuid);
        for (Deck::Card crd : pd.cards)
            player->add_cards(crd);
        player->set_rank(pd.prize);
        player->mutable_dun0()->set_brand(static_cast<int32_t>(pd.dun[0].b));
        player->mutable_dun0()->set_point(static_cast<int32_t>(pd.dun[0].point));
        player->mutable_dun0()->set_score(static_cast<int32_t>(pd.dun_prize[0]));
        player->mutable_dun1()->set_brand(static_cast<int32_t>(pd.dun[1].b));
        player->mutable_dun1()->set_point(static_cast<int32_t>(pd.dun[1].point));
        player->mutable_dun1()->set_score(static_cast<int32_t>(pd.dun_prize[1]));
        player->mutable_dun2()->set_brand(static_cast<int32_t>(pd.dun[2].b));
        player->mutable_dun2()->set_point(static_cast<int32_t>(pd.dun[2].point));
        player->mutable_dun2()->set_score(static_cast<int32_t>(pd.dun_prize[2]));
        player->mutable_spec()->set_brand(static_cast<int32_t>(pd.spec));
        player->mutable_spec()->set_point(static_cast<int32_t>(0));
        player->mutable_spec()->set_score(static_cast<int32_t>(0));
    }
    sendToAll(sndCode, snd);
    LOG_TRACE("G13, 单轮结算结束, round={}/{}, roomid={}", m_rounds, m_attr.rounds, getId());

    bool isCardMode = typeid(*this) == typeid(Water13_Card);
    if(isCardMode)
    {
        m_status = GameStatus::settle;
        m_settleTime = componet::toUnixTime(s_timerTime);

        PROTO_VAR_PUBLIC(S_G13_SettleTime, snd3);
        time_t elapse = componet::toUnixTime(s_timerTime) - m_settleTime;
        snd3.set_remain_seconds(MAX_WAIT_NEXT_DURATION > elapse ? MAX_WAIT_NEXT_DURATION - elapse : 0);
        sendToAll(snd3Code, snd3);

        //尝试总结算            不需要等待时间, 走结算流程
        trySettleAll();
    }
    else
    {
        //金币场模式, 重置状态为未准备状态, 金币场没有轮数
        for (PlayerInfo& info : m_players)
        {
            info.status = PublicProto::S_G13_PlayersInRoom::PREP;
        }
        m_status = GameStatus::prepare;
        m_rounds = 0;
    }

    syncAllPlayersInfoToAllClients();

}

void Water13::trySettleAll()
{
    //总结算
    if (m_status != GameStatus::settle)
        return;
    if (m_rounds < m_attr.rounds)
        return;
    settleAll();
}

void Water13::settleAll()
{
    //统计
    struct FinalCount
    {
        ClientUniqueId cuid = 0;
        int32_t win = 0;
        int32_t daqiang = 0;
        int32_t quanleida = 0;
        int32_t rank = 0;
    };

    auto finalCountGreater = [](const FinalCount& fc1, const FinalCount& fc2)
    {
        return fc1.rank > fc2.rank;
    };

    std::vector<FinalCount> allFinalCount(m_players.size());
    if (m_settleData.empty())
    {
        for (auto i = 0u; i < m_players.size(); ++i)
        {
            auto& count = allFinalCount[i];
            count.cuid = m_players[i].cuid;
        }
    }
    else
    {
        for (const auto& round : m_settleData)
        {
            for (auto i = 0u; i < round->players.size(); ++i)
            {
                const auto& pd = round->players[i];
                auto& count = allFinalCount[i];
                count.cuid = pd.cuid;
                count.win += pd.prize > 0 ? 1 : 0;
                bool daqiang = 0;
                for(const auto& item : pd.losers)
                    daqiang += item.second[1];
                count.daqiang += daqiang;
                if (pd.quanLeiDa)
                    count.quanleida += 1;
                count.rank += pd.prize;
            }
        }
    }

    //生成统计消息
    PROTO_VAR_PUBLIC(S_G13_AllRounds, sndFinal)
    for (const auto& count : allFinalCount)
    {
        auto player = sndFinal.add_players();
        player->set_cuid(count.cuid);
        player->set_win(count.win);
        player->set_daqiang(count.daqiang);
        player->set_quanleida(count.quanleida);
        player->set_rank(count.rank);
    }
    sendToAll(sndFinalCode, sndFinal);
    LOG_TRACE("G13, 总结算完成 round={}/{}, roomid={}", m_rounds, m_attr.rounds, getId());
    //扣钱
    if (m_attr.payor == PAY_WINNER)
    {
        std::sort(allFinalCount.begin(), allFinalCount.end(), finalCountGreater);
        //找到排名靠前的并列名次的最后一个
        uint32_t lastBigWinnerIndex = 0;
        for (uint32_t i = 1; i < allFinalCount.size(); ++i)
        {
            if (allFinalCount[lastBigWinnerIndex].rank > allFinalCount[i].rank)
                break;
            lastBigWinnerIndex = i;
        }

        const uint32_t bigwinnerSize = lastBigWinnerIndex + 1;
        const int32_t price = m_attr.playerPrice * m_attr.playerSize / bigwinnerSize;
        for (uint32_t i = 0; i < bigwinnerSize; ++i)
        {
            auto winner = ClientManager::me().getByCuid(allFinalCount[i].cuid);
            if (winner == nullptr)
            {//理论上不可能, 安全期间出日志
                LOG_ERROR("G13, 游戏结束赢家扣费, 失败, 不在线, 应扣money={}, 大赢家总数={}, cuid={}, openid={}", price, bigwinnerSize, winner->cuid(), winner->openid());
                continue;
            }
            winner->addMoney(-price);
            LOG_TRACE("G13, 游戏结束赢家扣费, 成功, moneyChange={}, 大赢家总数={}, cuid={}, openid={}", -price, bigwinnerSize, winner->cuid(), winner->openid());
        }
    }

    //房间状态更新
    m_status = GameStatus::settleAll;
    m_settleAllTime = componet::toUnixTime(s_timerTime);
    return;
}

uint32_t Water13::getEmptySeatIndex()
{
    for (uint32_t i = 0; i < m_players.size(); ++i)
    {
        if (m_players[i].cuid == 0)
            return i;
    }
    return NO_POS;
}

Water13::PlayerInfo* Water13::getPlayerInfoByCuid(ClientUniqueId cuid)
{
    for (PlayerInfo& info : m_players)
    {
        if (info.cuid == cuid)
            return &info;
    }
    return nullptr;
}

void Water13::timerExec()
{
    switch (m_status)
    {
        case GameStatus::prepare:
            {
                tryStartRound(); //全就绪可能因为某人不在线而无法开始游戏, 这里需要自动重试
            }
            break;
        case GameStatus::quickStart:
            {
                time_t elapse = componet::toUnixTime(s_timerTime) - m_quickStartTime;
                if (elapse >= MAX_VOTE_QUICK_START_DURATION)
                {
                    LOG_TRACE("申请提前开始投票超时, 游戏自动开始, roomid={}", getId());
                    for (PlayerInfo& info : m_players)
                    {
                        auto client = ClientManager::me().getByCuid(info.cuid);
                        if (client != nullptr)
                            client->noticeMessageBox("投票结束, 游戏开始!");
                        info.vote = PublicProto::VS_NONE;
                    }
                    m_quickStartTime = 0;
                    tryStartRound();
                }
            }
            break;
        case GameStatus::selBanker:
            {
                time_t elapse = componet::toUnixTime(s_timerTime) - m_selBankerTime;
                if (elapse >= MAX_SELECT_BANKER_DURATION)
                {
                    LOG_TRACE("下注超时, 自动选择最低档次, roomid={}", getId());
                    for (PlayerInfo& info : m_players)
                    {
                        //auto client = ClientManager::me().getByCuid(info.cuid);
                        //if (client != nullptr)
                        //    client->noticeMessageBox("下注结束, 选择庄家中!");
                        if(info.chipIn == 0)
                            info.chipIn = 1;
                    }
                    m_selBankerTime = 0;
                    checkAllChipInPoint();
                }
            }
            break;
        case GameStatus::selMultiple:
            {
                time_t elapse = componet::toUnixTime(s_timerTime) - m_selMultipleTime;
                if (elapse >= MAX_SELECT_MULTIPLE_DURATION)
                {
                    LOG_TRACE("选择倍率超时, 自动选择最低倍率, roomid={}", getId());
                    for (PlayerInfo& info : m_players)
                    {
                        //auto client = ClientManager::me().getByCuid(info.cuid);
                        //if (client != nullptr)
                        //    client->noticeMessageBox("选择倍率结束, 开始游戏!");
                        if(info.multiple == 0)
                            info.multiple = 1;
                    }
                    m_selMultipleTime = 0;
                    checkAllSelectMultiple();
                }
            }
            break;
        case GameStatus::play:
            {
            }
            break;
        case GameStatus::vote:
            {
                time_t elapse = componet::toUnixTime(s_timerTime) - m_startVoteTime;
                if (elapse >= MAX_VOTE_DURATION)
                {
                    LOG_TRACE("投票超时, 游戏终止, roomid={}", getId());
                    for (PlayerInfo& info : m_players)
                    {
                        auto client = ClientManager::me().getByCuid(info.cuid);
                        if (client != nullptr) client->noticeMessageBox("投票结束, 游戏解散!");
                        info.vote = PublicProto::VT_NONE;
                    }
                    m_startVoteTime = 0;
                    abortGame();
                }
            }
            break;
        case GameStatus::settle:
            {
                time_t elapse = componet::toUnixTime(s_timerTime) - m_settleTime;
                if (elapse >= MAX_WAIT_NEXT_DURATION)
                {
                    LOG_TRACE("倒计时时间已到, 直接开始下一轮, roomid={}", getId());
                    for (PlayerInfo* info : getValidPlayers())
                    {
                        //TODO 这里状态好像赋错了
                        info->status = PublicProto::S_G13_PlayersInRoom::READY;
                    }
                    m_settleTime = 0;
                    tryStartRound();
                }
            }
            break;
        case GameStatus::settleAll:
            {
                time_t elapse = componet::toUnixTime(s_timerTime) - m_settleAllTime;
                if (elapse >= MAX_WAIT_OVER_DURATION)
                {
                    LOG_TRACE("总结算等待3分钟,没有人续局,游戏终止, roomid={}", getId());
                    m_settleAllTime = 0;
                    abortGame();
                }
            }
            break;
        case GameStatus::closed:
            {
                destroyLater();
            }
    }
    return;
}

bool Water13::hasMaPai(Deck::Card* c, int32_t size, int32_t maPai)
{
    for(int i = 0; i < size; ++i)
    {
        if(c[i] == maPai)
        {
            return true;
        }
    }
    return false;
}

template<typename Player>
Water13::RoundSettleData::Ptr Water13::calcRound(const std::vector<Player>& playerList, int32_t daQiang, int32_t maPai, bool quanLeiDa)
{
    auto rsd = RoundSettleData::create();
    rsd->players.reserve(playerList.size());
    for (const auto& info : playerList)
    {
        rsd->players.emplace_back();
        auto& data = rsd->players.back();
        data.cuid = info.cuid;
        data.cards = info.cards;

        //1墩
        data.dun[0] = Deck::brandInfo(data.cards.data(), 3);
        data.dun[1] = Deck::brandInfo(data.cards.data() + 3, 5);
        data.dun[2] = Deck::brandInfo(data.cards.data() + 8, 5);
        if (info.cardsSpecBrand)
            data.spec = Deck::g13SpecialBrand(data.cards.data(), data.dun[1].b, data.dun[2].b);
        data.prize = 0;
        if(hasMaPai(data.cards.data(), 13, maPai))
        {
            data.hasMaPai = true;
        }
    }

    auto& datas = rsd->players;
    for (uint32_t i = 0; i < datas.size(); ++i)
    {
        auto& dataI = datas[i];
        for (uint32_t j = i + 1; j < datas.size(); ++j)
        {
            auto& dataJ = datas[j];
            ///////////////////////////////以下为特殊牌型//////////////////////////

            if (dataI.spec != Deck::G13SpecialBrand::none || 
                dataJ.spec != Deck::G13SpecialBrand::none)
            {
                auto specCmpValueI = underlying(dataI.spec) % 10;
                auto specCmpValueJ = underlying(dataJ.spec) % 10;
                if (specCmpValueI == specCmpValueJ) //平局
                    continue;

                auto& specWinner = (specCmpValueI > specCmpValueJ) ?  dataI : dataJ;
                int32_t specPrize = 0;
                switch (specWinner.spec)
                {
                case Deck::G13SpecialBrand::flushStriaght: //11.   清龙（同花十三水）：若大于其他玩家，每家赢取104分
                    specPrize = 104;
                    break;
                case Deck::G13SpecialBrand::straight:
                    specPrize = 52;
                    break;
                case Deck::G13SpecialBrand::royal:
                case Deck::G13SpecialBrand::tripleStraightFlush:
                case Deck::G13SpecialBrand::tripleBombs:
                    specPrize = 26;
                    break;
                case Deck::G13SpecialBrand::allBig:
                case Deck::G13SpecialBrand::allLittle:
                case Deck::G13SpecialBrand::redOrBlack:
                case Deck::G13SpecialBrand::quradThreeOfKind:
                case Deck::G13SpecialBrand::pentaPairsAndThreeOfKind:
                case Deck::G13SpecialBrand::sixPairs:
                case Deck::G13SpecialBrand::tripleStraight:
                case Deck::G13SpecialBrand::tripleFlush:
                    specPrize = 6;
                    break;
                case Deck::G13SpecialBrand::none:
                    break;
                default:
                    break;
                }

                if (specCmpValueI > specCmpValueJ)
                {
                    specWinner.losers[j][0] = specPrize;
                    specWinner.losers[j][1] = 0;
                }
                else
                {
                    specWinner.losers[i][0] = specPrize;
                    specWinner.losers[i][1] = 0;
                }
                //本轮已经不用再比了, 因为至少有一家是特殊牌型, 特殊的都大于一般的
                continue;
            }


            /////////////////////////////////////以下为一般牌型//////////////////////
            int32_t dunCmps[] = 
            {
                Deck::cmpBrandInfo(dataI.dun[0], dataJ.dun[0]),
                Deck::cmpBrandInfo(dataI.dun[1], dataJ.dun[1]),
                Deck::cmpBrandInfo(dataI.dun[2], dataJ.dun[2]),
            };

            //下面按照按照逐个规则计算3墩的胜负分
            int32_t dunPrize[3] = {0, 0, 0}; //3墩胜负分的记录, I赢记整数, J赢记负数, 最后看正负知道谁赢了, 绝对值代表赢的数量

            // rule 1, 同一墩赢1个玩家1水 +1分
            // rule 2, 同一墩输1个玩家1水 -1分
            // rule 3, 同一墩和其它玩家打和（牌型大小一样）0分
            for (uint32_t d = 0; d < 3; ++d)
            {
                switch (dunCmps[d])
                {
                case 1: //I赢
                    dunPrize[d] += 1;
                    break;
                case 2: //J赢
                    dunPrize[d] -= 1;
                    break;
                case 0: //平
                default:
                    break;
                }
            }
            {// rule 4.  冲三：头墩为三张点数一样的牌且大于对手，记1分+2分奖励，共3分
                const uint32_t d = 0;
                if (dunCmps[d] != 0)
                {
                    const uint32_t extra = 2;
                    if (dunCmps[d] == 1 && dataI.dun[d].b == Deck::Brand::threeOfKind)
                    {
                        dunPrize[d] += extra;
                    }
                    else if (dunCmps[d] == 2 && dataJ.dun[d].b == Deck::Brand::threeOfKind)
                    {
                        dunPrize[d] -= extra;
                    }
                }
            }
            {// 5, 中墩葫芦：中墩为葫芦且大于对手，记1分+1分奖励，共2分
                const uint32_t d = 1;
                if (dunCmps[d] != 0)
                {
                    const uint32_t extra = 1;
                    if (dunCmps[d] == 1 && dataI.dun[d].b == Deck::Brand::fullHouse)
                    {
                        dunPrize[d] += extra;
                    }
                    else if (dunCmps[d] == 2 && dataJ.dun[d].b == Deck::Brand::fullHouse)
                    {
                        dunPrize[d] -= extra;
                    }
                }
            }
            {//6, 五同：
                //中墩为五同且大于对手，记1分+19分奖励，共20分
                uint32_t d = 1;
                if (dunCmps[d] != 0)
                {
                    const uint32_t extra = 19;
                    if (dunCmps[d] == 1 && dataI.dun[d].b == Deck::Brand::fiveOfKind)
                    {
                        dunPrize[d] += extra;
                    }
                    else if (dunCmps[d] == 2 && dataJ.dun[d].b == Deck::Brand::fiveOfKind)
                    {
                        dunPrize[d] -= extra;
                    }
                }

                //尾墩为五同且大于对手，记1分+9分奖励，共10分
                d = 2;
                if (dunCmps[d] != 0)
                {
                    const uint32_t extra = 9;
                    if (dunCmps[d] == 1 && dataI.dun[d].b == Deck::Brand::fiveOfKind)
                    {
                        dunPrize[d] += extra;
                    }
                    else if (dunCmps[d] == 2 && dataJ.dun[d].b == Deck::Brand::fiveOfKind)
                    {
                        dunPrize[d] -= extra;
                    }
                }

            }
            
            {//7.   同花顺
                //中墩为同花顺且大于对手，记1分+9分奖励，共10分
                uint32_t d = 1;
                if (dunCmps[d] != 0)
                {
                    const uint32_t extra = 9;
                    if (dunCmps[d] == 1 && dataI.dun[d].b == Deck::Brand::straightFlush)
                    {
                        dunPrize[d] += extra;
                    }
                    else if (dunCmps[d] == 2 && dataJ.dun[d].b == Deck::Brand::straightFlush)
                    {
                        dunPrize[d] -= extra;
                    }
                }

                //尾墩为同花顺且大于对手，记1分+4分奖励，共5分
                d = 2;
                if (dunCmps[d] != 0)
                {
                    const uint32_t extra = 4;
                    if (dunCmps[d] == 1 && dataI.dun[d].b == Deck::Brand::straightFlush)
                    {
                        dunPrize[d] += extra;
                    }
                    else if (dunCmps[d] == 2 && dataJ.dun[d].b == Deck::Brand::straightFlush)
                    {
                        dunPrize[d] -= extra;
                    }
                }
            }
            {//8.   铁支, 四条
                //中墩为铁支且大于对手，记1分+7分奖励，共8分
                uint32_t d = 1;
                if (dunCmps[d] != 0)
                {
                    const uint32_t extra = 7;
                    if (dunCmps[d] == 1 && dataI.dun[d].b == Deck::Brand::fourOfKind)
                    {
                        dunPrize[d] += extra;
                    }
                    else if (dunCmps[d] == 2 && dataJ.dun[d].b == Deck::Brand::fourOfKind)
                    {
                        dunPrize[d] -= extra;
                    }
                }

                //尾墩为为铁支且大于对手，记1分+3分奖励，共4分
                d = 2;
                if (dunCmps[d] != 0)
                {
                    const uint32_t extra = 3;
                    if (dunCmps[d] == 1 && dataI.dun[d].b == Deck::Brand::fourOfKind)
                    {
                        dunPrize[d] += extra;
                    }
                    else if (dunCmps[d] == 2 && dataJ.dun[d].b == Deck::Brand::fourOfKind)
                    {
                        dunPrize[d] -= extra;
                    }
                }
            }

            //累加基础分值
            for(int z = 0; z < 3; ++z)
            {
                dataI.dun_prize[z] += dunPrize[z];
            }

            //9, 打枪
            //如果三墩都比一个玩家大的话，向该玩家收取分数*2,包含特殊分
            //诸如冲三后打枪一个玩家，为5分+5分，共对该玩家收取10分
            const uint32_t prize = dunPrize[0] + dunPrize[1] + dunPrize[2];
            if (daQiang && (dunPrize[0] > 0 && dunPrize[1] > 0 && dunPrize[2] > 0) ) //I打枪
            {
                dataI.losers[j][0] = prize;
                dataI.losers[j][1] = 1;
            }
            else if (daQiang && (dunPrize[0] < 0 && dunPrize[1] < 0 && dunPrize[2] < 0) ) //J打枪
            {
                dataJ.losers[i][0] = -prize;
                dataJ.losers[i][1] = 1;
            }
            else //没有打枪
            {
                if (prize == 0) //平局
                    continue;

                if (prize > 0) //I胜利
                {
                    dataI.losers[j][0] = prize;
                    dataI.losers[j][1] = 0;
                }
                else //J胜利
                {
                    dataJ.losers[i][0] = -prize;
                    dataJ.losers[i][1] = 0;
                }
            }
        }
    }
    //两两比完了, 最终结果把每一次pk都算上
    //10, 全垒打, 计算完每家打枪的分数后，再*2，也就是总分X分+X分
    {
        for (auto d = 0u; d < datas.size(); ++d)
        {
            auto& winner = datas[d];

            //判断是否是全垒打
            winner.quanLeiDa = false;
            if ((quanLeiDa)  //全垒打启用
                && (playerList.size() > 2) //两人房无全垒打
                && (winner.losers.size() + 1 == datas.size()) ) //全胜
            {
                winner.quanLeiDa = true;
                for (auto iter = winner.losers.begin(); iter != winner.losers.end(); ++iter)
                {
                    if (iter->second[1] == 0)
                    {
                        winner.quanLeiDa = false;
                        break;
                    }
                }
            }

            //开始对losers逐人结算分数
            for (auto iter = winner.losers.begin(); iter != winner.losers.end(); ++iter)
            {
                auto& loser = datas[iter->first];
                int32_t prize = iter->second[0]; //分数得失
                if (iter->second[1] > 0) //打枪
                {
                    if (daQiang == DQ_X_DAO)
                        prize += GameConfig::me().data().shootAddPoint;
                    else
                        prize *= 2;
                }
                //包含马牌,输赢积分翻倍
                if(winner.hasMaPai || loser.hasMaPai)
                {
                    prize *= 2;
                }
                if (winner.quanLeiDa) //全垒打
                {
                    prize *= 2;
                }
                winner.prize += prize;
                loser.prize -= prize;
            }
        }       
    }
    return rsd;
}

//判断v2是不是v1的子集
bool Water13::isSubset(std::vector<int32_t> v1, std::vector<int32_t> v2)
{
    int i = 0, j = 0;
    int m = v1.size();
    int n = v2.size();

    if(m < n)
        return 0;

    std::sort(v1.begin(), v1.end());
    std::sort(v2.begin(), v2.end());

    while(i < n && j < m){
        if(v1[j] < v2[i])
        {
            j++;
        }
        else if(v1[j] == v2[i]){
            j++;
            i++;
        }
        else if(v1[j] > v2[i]){
            return 0;
        }
    }

    if(i < n){
        return 0;
    }
    else{
        return 1;
    }
}

void Water13::proto_C_G13_ChipInPoint(ProtoMsgPtr proto, ClientConnectionId ccid)
{
    auto client = ClientManager::me().getByCcid(ccid);
    if (client == nullptr)
        return;

    if (client->roomid() == 0)
    {
        client->noticeMessageBox("已经不在房间内了");
        return;
    }

    auto game = getByRoomId(client->roomid());
    if (game == nullptr)
    {
        LOG_ERROR("ChipInPoint, client上记录的房间号不存在, roomid={}, name={}, ccid={}, cuid={}, openid={}", 
                  client->roomid(), client->name(), client->ccid(), client->cuid(), client->openid());
        client->afterLeaveRoom(); //ERR_HANDLER
        return;
    }

    if(game->m_status !=  GameStatus::selBanker)
        return;

    auto rcv = PROTO_PTR_CAST_PUBLIC(C_G13_ChipInPoint, proto);

    PlayerInfo* info = game->getPlayerInfoByCuid(client->cuid());
    if (info == nullptr)
    {
        return;
    }

    //检测点数是否过大
    if(rcv->point() > game->m_attr.bankerMultiple)
    {
        return;
    }

    info->chipIn = rcv->point();
    game->checkAllChipInPoint();
}

void Water13::checkAllChipInPoint()
{
    if (m_status != GameStatus::selBanker)
        return;

    PROTO_VAR_PUBLIC(S_G13_ChipInInfo, snd);
    time_t elapse = componet::toUnixTime(s_timerTime) - m_selBankerTime;
    snd.set_remain_seconds(MAX_SELECT_BANKER_DURATION > elapse ? MAX_SELECT_BANKER_DURATION - elapse : 0);
    uint32_t size = 0;
    int32_t maxPoint = 0;
    PlayerInfo* maxPlayer = NULL;

    //检测所有有效玩家的选择
    std::vector<PlayerInfo*> vec = getValidPlayers();
    for (PlayerInfo* info : vec)
    {
        if (info->chipIn > 0)
        {
            ++size;
            auto chipInfo = snd.add_list();
            chipInfo->set_cuid(info->cuid);
            chipInfo->set_multiple(info->chipIn);
        }
        if(info->chipIn > maxPoint)
        {
            maxPoint = info->chipIn;
            maxPlayer = info;
        }
    }

    if (size == vec.size()) //全部下注完成
    {
        if(maxPoint == 0)
            return;

        //筛选庄家 & 重置选择
        bool needRandom = true;
        for (PlayerInfo* info : vec)
        {
            if(maxPoint != info->chipIn)
            {
                needRandom = false;
            }
            //比对完重置
            info->chipIn = 0;
        }

        if(needRandom)
        {
            //抢庄倍率相同,随机产生一个
            componet::Random<int32_t> rander(1, vec.size());
            maxPlayer = vec[rander.get() - 1];
        }

		if(maxPlayer != NULL)
		{
			m_bankerCuid = maxPlayer->cuid;
			//广播庄家信息
			PROTO_VAR_PUBLIC(S_G13_BankerInfo, snd2);
			snd2.set_cuid(m_bankerCuid);
			sendToAll(snd2Code, snd2);

            //状态切换
            m_selMultipleTime = componet::toUnixTime(s_timerTime);
            m_status = GameStatus::selMultiple;

            for (PlayerInfo* info : vec)
            {
                info->status = PublicProto::S_G13_PlayersInRoom::SELMULTIPLE;
            }
            syncAllPlayersInfoToAllClients();

            PROTO_VAR_PUBLIC(S_G13_MultipleInfo, snd3);
            time_t elapse = componet::toUnixTime(s_timerTime) - m_selMultipleTime;
            snd3.set_remain_seconds(MAX_SELECT_MULTIPLE_DURATION > elapse ? MAX_SELECT_MULTIPLE_DURATION - elapse : 0);
            for (PlayerInfo* info : getValidPlayers())
            {
                auto chipInfo = snd3.add_list();
                chipInfo->set_cuid(info->cuid);
                chipInfo->set_multiple(info->multiple);
            }
            sendToAll(snd3Code, snd3);
		}
        m_selBankerTime = 0;
    }
    else
    {
        //更新选择情况
        for (PlayerInfo& info : m_players)
        {
            auto client = ClientManager::me().getByCuid(info.cuid);
            if (client != nullptr)
                client->sendToMe(sndCode, snd);
        }
    }
}

//获取有效玩家
std::vector<Water13::PlayerInfo*> Water13::getValidPlayers()
{
    std::vector<PlayerInfo*> vec;
    for(PlayerInfo& info : m_players)
    {
        if (info.cuid == 0 || info.status == PublicProto::S_G13_PlayersInRoom::WITNESS)
            continue;

        vec.push_back(&info);
    }
    return vec;
}

void Water13::proto_C_G13_SelectMultiple(ProtoMsgPtr proto, ClientConnectionId ccid)
{
    auto client = ClientManager::me().getByCcid(ccid);
    if (client == nullptr)
        return;

    if (client->roomid() == 0)
    {
        client->noticeMessageBox("已经不在房间内了");
        return;
    }

    auto game = getByRoomId(client->roomid());
    if (game == nullptr)
    {
        LOG_ERROR("SelectMultiple, client上记录的房间号不存在, roomid={}, name={}, ccid={}, cuid={}, openid={}", 
                  client->roomid(), client->name(), client->ccid(), client->cuid(), client->openid());
        client->afterLeaveRoom(); //ERR_HANDLER
        return;
    }

    if(game->m_status !=  GameStatus::selMultiple)
        return;

    auto rcv = PROTO_PTR_CAST_PUBLIC(C_G13_SelectMultiple, proto);

    PlayerInfo* info = game->getPlayerInfoByCuid(client->cuid());
    if (info == nullptr)
    {
        return;
    }

    //检测倍率是否过大
    if(rcv->point() > game->m_attr.bankerMultiple)
    {
        return;
    }

    info->multiple = rcv->point();
    game->checkAllSelectMultiple();
}

void Water13::checkAllSelectMultiple()
{
    if (m_status != GameStatus::selMultiple)
        return;

    PROTO_VAR_PUBLIC(S_G13_MultipleInfo, snd);
    time_t elapse = componet::toUnixTime(s_timerTime) - m_selMultipleTime;
    snd.set_remain_seconds(MAX_SELECT_MULTIPLE_DURATION > elapse ? MAX_SELECT_MULTIPLE_DURATION - elapse : 0);
    uint32_t size = 0;

    //检测所有有效玩家的选择
    std::vector<PlayerInfo*> vec = getValidPlayers();
    for (PlayerInfo* info : vec)
    {
        if (info->multiple > 0)
        {
            ++size;
            auto multiInfo = snd.add_list();
            multiInfo->set_cuid(info->cuid);
            multiInfo->set_multiple(info->multiple);
        }
    }

    if (size == vec.size() - 1) //全部选择完成      //庄家不用选
    {
        //发完剩余的牌,进入摆牌阶段
        PROTO_VAR_PUBLIC(S_G13_PlayersInRoom, snd1);
        snd1.set_rounds(m_rounds);
        int idx = 0;
        for (PlayerInfo* info : vec)
        {
            //补发后面的牌
            for(int i = 8; i < 13; ++i)
            {
                info->cards[i] = remainBrands[idx++];
                //排个序, 以后检查外挂换牌时好比较
                std::sort(info->cards.begin(), info->cards.end());
            }

            //同步手牌到端
            PROTO_VAR_PUBLIC(S_G13_HandOfMine, snd2)
            snd2.set_rounds(m_rounds);
            std::string cardsStr;
            cardsStr.reserve(42);
            for (auto card : info->cards)
            {
                cardsStr.append(std::to_string(card));
                cardsStr.append(",");
                snd2.add_cards(card);
            }
            auto client = ClientManager::me().getByCuid(info->cuid);
            if (client != nullptr)
            {
                client->sendToMe(snd2Code, snd2);
            }

            info->status = PublicProto::S_G13_PlayersInRoom::SORT;

            auto player = snd1.add_players();
            player->set_status(info->status);
            player->set_cuid(info->cuid);
            player->set_name(info->name);
            player->set_imgurl(info->imgurl);
            player->set_ipstr(info->ipstr);
            player->set_rank(info->rank);
            LOG_TRACE("deal deck, roomid={}, round={}/{}, cuid={}, name={}, cards=[{}]", getId(), m_rounds, m_attr.rounds, info->cuid, info->name, cardsStr);
        }
        sendToAll(snd1Code, snd1);

        m_status = GameStatus::play;
        m_selMultipleTime = 0;
        remainBrands.clear();
    }
    else
    {
        //更新选择情况(告知所有人,包括观战的)
        for (PlayerInfo& info : m_players)
        {
            auto client = ClientManager::me().getByCuid(info.cuid);
            if (client != nullptr)
                client->sendToMe(sndCode, snd);
        }
    }
}

int32_t Water13::getPlayerSize()
{
    int32_t sum = 0;
    for (const auto& playerInfo : m_players)
    {
        if(playerInfo.cuid != 0)
            sum++;
    }
    return sum;
}

Water13::~Water13()
{
}

}
