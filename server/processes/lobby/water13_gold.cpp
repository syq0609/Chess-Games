#include "water13_gold.h"
#include "client.h"

#include "game_config.h"

#include "dbadaptcher/redis_handler.h"

#include "componet/logger.h"
#include "componet/scope_guard.h"
#include "componet/tools.h"
#include "componet/format.h"
#include "componet/random.h"

#include "protocol/protobuf/public/client.codedef.h"

#include <random>
#include <algorithm>

namespace lobby{

std::map<int32_t, std::vector<Water13::Ptr>> Water13_Gold::roomsArr;       //房间列表

void Water13_Gold::regMsgHandler()
{
    using namespace std::placeholders;
    REG_PROTO_PUBLIC(C_G13Gold_GetOpenPlace, std::bind(&Water13_Gold::proto_C_G13Gold_GetOpenPlace, _1, _2));
    REG_PROTO_PUBLIC(C_G13_EnterGoldPlace, std::bind(&Water13_Gold::proto_C_G13_EnterGoldPlace, _1, _2));
    REG_PROTO_PUBLIC(C_G13Gold_ChangeRoom, std::bind(&Water13_Gold::proto_C_G13Gold_ChangeRoom, _1, _2));
    REG_PROTO_PUBLIC(C_G13Gold_QuickStart, std::bind(&Water13_Gold::proto_C_G13Gold_QuickStart, _1, _2));
}

void Water13_Gold::proto_C_G13Gold_GetOpenPlace(ProtoMsgPtr proto, ClientConnectionId ccid)
{
    auto client = ClientManager::me().getByCcid(ccid);
    if (client == nullptr)
        return;

    if (client->roomid() != 0)
    {
        client->noticeMessageBox("已经在房间内了");
        return;
    }

    PROTO_VAR_PUBLIC(S_G13Gold_OpenPlaceInfo, snd);

    //读取配置发送给客户端
    auto ruleCfg = GameConfig::me().data().water13RoomRule;
    for(auto& iter : ruleCfg)
    {
        auto placeList = snd.add_place_list();
        placeList->set_id(iter.first);
        auto& cfg = iter.second;
        placeList->set_player_size(getPlayerSizeByType(iter.first));
        placeList->set_score(cfg.score);
        placeList->set_max(cfg.max);
    }
    client->sendToMe(sndCode, snd);
}

void Water13_Gold::proto_C_G13_EnterGoldPlace(ProtoMsgPtr proto, ClientConnectionId ccid)
{
    auto client = ClientManager::me().getByCcid(ccid);
    if (client == nullptr)
        return;

    if (client->roomid() != 0)
    {
        client->noticeMessageBox("已经在房间中了");
        return;
    }

    auto rcv = PROTO_PTR_CAST_PUBLIC(C_G13_EnterGoldPlace, proto);
    auto ruleCfg = GameConfig::me().data().water13RoomRule;
    std::map<int32_t, GameConfigData::water13RoomRuleInfo>::iterator it = ruleCfg.find(rcv->place_id());
    if(it == ruleCfg.end())
        return;

    //检测进入条件
    if(!client->enoughMoney1(it->second.min))
    {
        client->noticeMessageBox("您的金币不足，无法进入");
        return;
    }

    if(client->money1() > it->second.max)
    {
        client->noticeMessageBox("金币超过最大限制");
        return;
    }
    
    matchRoom(rcv->place_id(), client);
}

void Water13_Gold::proto_C_G13Gold_ChangeRoom(ProtoMsgPtr proto, ClientConnectionId ccid)
{
    auto client = ClientManager::me().getByCcid(ccid);
    if (client == nullptr)
        return;

    if (client->roomid() == 0)
    {
        client->noticeMessageBox("你不在房间中");
        return;
    }

    auto game = getByRoomId(client->roomid());
    if (game == nullptr)
    {
        LOG_ERROR("ChangeRoom, client上记录的房间号不存在, roomid={}, name={}, ccid={}, cuid={}, openid={}",
                client->roomid(), client->name(), client->ccid(), client->cuid(), client->openid());        client->afterLeaveRoom(); //ERR_HANDLER
        return;
    }
    matchRoom(game->m_attr.placeType, client);
}

void Water13_Gold::proto_C_G13Gold_QuickStart(ProtoMsgPtr proto, ClientConnectionId ccid)
{
    auto client = ClientManager::me().getByCcid(ccid);
    if (client == nullptr)
        return;

    if (client->roomid() != 0)
    {
        client->noticeMessageBox("你已经在房间中");
        return;
    }

    auto& ruleCfg = GameConfig::me().data().water13RoomRule;
    auto rIter = ruleCfg.rbegin();
    for(; rIter != ruleCfg.rend(); rIter++)
    {
        if(client->enoughMoney1(rIter->second.min) && client->money1() < rIter->second.max)
        {
            matchRoom(rIter->first, client);
            break;
        }
    }
}

void Water13_Gold::createGame(ClientPtr client, int32_t placeId)
{
    if (client == nullptr)
        return;

    if (client->roomid() != 0)
    {
        client->noticeMessageBox("已经在房间中, 无法创建新房间!");
        return;
    }
    
    auto ruleCfg = GameConfig::me().data().water13RoomRule;
    std::map<int32_t, GameConfigData::water13RoomRuleInfo>::iterator it = ruleCfg.find(placeId);
    if(it == ruleCfg.end())
        return;

    {//检查创建消息中的数据是否合法
        int32_t attrCheckResult = 0;
        do {
            if (it->second.playType != GP_NORMAL && it->second.playType != GP_BANKER)
            {
                attrCheckResult = 1;
                break;
            }

            const int32_t minPlayerSize = 2;
            const int32_t maxPlayerSize = 8;

            if (it->second.playerSize < minPlayerSize || it->second.playerSize > maxPlayerSize)
            {
                attrCheckResult = 2;
                break;
            }

            if (it->second.daQiang != DQ_NORMAL && it->second.daQiang != DQ_X_DAO)
            {
                attrCheckResult = 3;
                break;
            }

            if(it->second.maPai < 0 || it->second.maPai > 52)
            {
                attrCheckResult = 4;
                break;
            }

        } while (false);

        //是否出错
        if (attrCheckResult > 0)
        {
            client->noticeMessageBox(componet::format("创建房间失败, 非法的房间属性设定({}),配置错误!", attrCheckResult));
            return;
        }
    }

    //房间
    auto game = Water13_Gold::create(client->cuid(), GameType::water13_gold);
    if (Room::add(game) == false)
    {
        LOG_ERROR("G13, 创建房间失败, Room::add失败");
        return;
    }

    //初始化游戏信息
    auto& attr       = game->m_attr;
    attr.playType    = it->second.playType;
    attr.daQiang    = it->second.daQiang;
    attr.rounds      = 3;           //测试一下,金币场不该有回合概念
    attr.playerSize  = it->second.playerSize;
    attr.maPai      = it->second.maPai;
    attr.bankerMultiple  = it->second.bankerMultiple;
    attr.withGhost  = it->second.withGhost;
    attr.payor      = PAY_FREE;     //金币场自动创建,不要钱
    attr.goldPlace  = true;
    attr.placeType  = it->first;
    //其余不需要的属性走默认

    //玩家座位数量
    game->m_players.resize(attr.playerSize);

    //最后进房间, 因为进房间要预扣款, 进入后再有什么原因失败需要回退
    if (!game->enterRoom(client))
    {
        LOG_TRACE("建立房间失败, enterRoom 失败, name={}, cuid={}, openid={}", client->name(), client->cuid(), client->openid()); 
        return;
    }

    Water13_Gold::roomsArr[placeId].push_back(game);
    saveToDB(game, "create room", client);
}


void Water13_Gold::giveUp(ClientPtr client)
{
    if(client == nullptr)
        return;

    auto game = getByRoomId(client->roomid());
    if (game == nullptr)
    {
        LOG_ERROR("C_G13_GiveUp, client上记录的房间号不存在, roomid={}, name={}, ccid={}, cuid={}, openid={}", 
                  client->roomid(), client->name(), client->ccid(), client->cuid(), client->openid());
        client->afterLeaveRoom();
        return;
    }

    PlayerInfo* info = game->getPlayerInfoByCuid(client->cuid());
    if (info == nullptr)
    {
        client->afterLeaveRoom();
        return;
    }

    ON_EXIT_SCOPE_DO(saveToDB(game, "give up game", client));

    switch (game->m_status)
    {
        case GameStatus::prepare:
            {
                LOG_TRACE("十三水金币场 准备期间成员离开房间, roomid={}, name={},  ccid={}, cuid={}, openid={}",
                        game->getId(), client->name(), client->ccid(), client->cuid(), client->openid()); 
                game->removePlayer(client);
                if(game->m_players.size() == 0)
                {
                    //解散房间
                    game->abortGame();
                    auto roomIter = Water13_Gold::roomsArr.find(m_attr.placeType);
                    if(roomIter != Water13_Gold::roomsArr.end())
                    {
                        auto iter = roomIter->second;
                        auto beg = iter.begin();
                        for( ; beg != iter.end(); ++beg)
                        {
                            if((*beg)->getId() == getId())
                            {
                                iter.erase(beg);
                                break;
                            }
                        }
                    }
                }
            }
            break;
        case GameStatus::quickStart:
            break;
        case GameStatus::selBanker:
            break;
        case GameStatus::selMultiple:
            break;
        case GameStatus::play:
            break;
        case GameStatus::vote:
            break;
        case GameStatus::settle:
            break;
        case GameStatus::settleAll:
        case GameStatus::closed:
            {
                LOG_TRACE("结算完毕后主动离开房间, roomid={}, name={}, ccid={}, cuid={}, openid={}",
                        game->getId(), client->name(), client->ccid(), client->cuid(), client->openid()); 
                game->removePlayer(client);
                return;
            }
    }
    return;
}

void Water13_Gold::readyFlag(const ProtoMsgPtr& proto, ClientPtr client)
{
    if(client == nullptr)
        return;

    auto game = getByRoomId(client->roomid());
    if (game == nullptr)
    {
        LOG_ERROR("C_G13_ReadyFlag, client上记录的房间号不存在, roomid={}, name={}, ccid={}, cuid={}, openid={}", 
                  client->roomid(), client->name(), client->ccid(), client->cuid(), client->openid());
        client->afterLeaveRoom();
        return;
    }

    PlayerInfo* info = game->getPlayerInfoByCuid(client->cuid());
    if (info == nullptr)
    {
        client->afterLeaveRoom();
        return;
    }

    if (info->status == PublicProto::S_G13_PlayersInRoom::WITNESS)
    {
        client->noticeMessageBox("房主未同意,不能准备");
        return;
    }

    if (game->m_status == GameStatus::quickStart)
    {
        client->noticeMessageBox("房主已申请快速开始,请选择");
        return;
    }

    if (game->m_status != GameStatus::prepare)
    {
        client->noticeMessageBox("游戏已开始");
        return;
    }

    auto rcv = PROTO_PTR_CAST_PUBLIC(C_G13_ReadyFlag, proto);
    auto oldStatus = info->status;
    auto newStatus = rcv->ready() ? PublicProto::S_G13_PlayersInRoom::READY : PublicProto::S_G13_PlayersInRoom::PREP;

    //状态没变, 不用处理了
    if (oldStatus == newStatus)
        return;

    //改变状态
    info->status = newStatus;
    LOG_TRACE("ReadyFlag, 玩家设置准备状态, readyFlag={}, roomid={}, name={}, ccid={}, cuid={}, openid={}",
              rcv->ready(), client->roomid(), client->name(), client->ccid(), client->cuid(), client->openid());

    game->syncAllPlayersInfoToAllClients();

    //新的ready确认, 检查是否所有玩家都已确认, 可以启动游戏
    if (newStatus == PublicProto::S_G13_PlayersInRoom::READY)
        game->tryStartRound();
    saveToDB(game, "set ready flag, start game", client);
}

void Water13_Gold::bringOut(const ProtoMsgPtr& proto, ClientPtr client)
{
    if(client == nullptr)
        return;

    auto game = getByRoomId(client->roomid());
    if (game == nullptr)
    {
        LOG_ERROR("C_G13_ReadyFlag, client上记录的房间号不存在, roomid={}, name={}, ccid={}, cuid={}, openid={}", client->roomid(), client->name(), client->ccid(), client->cuid(), client->openid());
        client->afterLeaveRoom();
        return;
    }

    PlayerInfo* info = game->getPlayerInfoByCuid(client->cuid());
    if (info == nullptr)
    {
        client->afterLeaveRoom();
        return;
    }

    if (game->m_status != GameStatus::play)
    {
        client->noticeMessageBox("当前不能出牌(0x01)");
        return;
    }

    if (info->status != PublicProto::S_G13_PlayersInRoom::SORT)
    {
         client->noticeMessageBox("当前不能出牌0x02");
         return;
    }

    auto rcv = PROTO_PTR_CAST_PUBLIC(C_G13_BringOut, proto);
    if(rcv->cards_size() != 13)
    {
        client->noticeMessageBox("出牌数量不对");
        return;
    }

    {//合法性判断
        std::array<Deck::Card, 13> tmp;
        std::copy(rcv->cards().begin(), rcv->cards().end(), tmp.begin());
        // 1, 是否是自己的牌
        std::sort(tmp.begin(), tmp.end());
        for (auto i = 0u; i < tmp.size(); ++i)
        {
            if (tmp[i] != info->cards[i])
            {
                client->noticeMessageBox("牌组不正确, 请正常游戏");
                return;
            }
        }
        // 2, 摆牌的牌型是否合法
        std::copy(rcv->cards().begin(), rcv->cards().end(), tmp.begin());
        auto dun0 = Deck::brandInfo(tmp.data(), 3);
        auto dun1 = Deck::brandInfo(tmp.data() + 3, 5);
        auto dun2 = Deck::brandInfo(tmp.data() + 8, 5);
        if (rcv->special())
        {
            //鬼牌不能组成特殊牌型
            if(Deck::hasGhost(tmp.data(), 13))
            {
                client->noticeMessageBox("不是特殊牌型, 请重新理牌");
                return;
            }

            auto spec = Deck::g13SpecialBrand(tmp.data(), dun1.b, dun2.b);
            if (spec == Deck::G13SpecialBrand::none)
            {
                client->noticeMessageBox("不是特殊牌型, 请重新理牌");
                return;
            }
            info->cardsSpecBrand = true;
        }
        else
        {
            if (Deck::cmpBrandInfo(dun0, dun1) == 1 || 
                Deck::cmpBrandInfo(dun1, dun2) == 1)
            {
                client->noticeMessageBox("相公啦, 请重新理牌");
                return;
            }
            info->cardsSpecBrand = false;
        }
    }
    //改变玩家状态, 接收玩家牌型数据
    info->status = PublicProto::S_G13_PlayersInRoom::COMPARE;
    std::string cardsStr;
    cardsStr.reserve(42);
    for (uint32_t index = 0; index < info->cards.size(); ++index)
    {
        info->cards[index] = rcv->cards(index);
        cardsStr.append(std::to_string(info->cards[index]));
        cardsStr.append(",");
    }
    LOG_TRACE("bring out, roomid={}, round={}/{}, name={}, cuid={}, openid={}, special={}, cards=[{}]",
             client->roomid(), game->m_rounds, game->m_attr.rounds, client->name(), client->cuid(), client->openid(), rcv->special(), cardsStr);
    saveToDB(game, "bring out", client);

    game->syncAllPlayersInfoToAllClients();

    game->trySettleGame();

    //顺利结算完毕, 开下一局
    if (game->m_status == GameStatus::settle)
        game->tryStartRound();
}

/*************************************************************************/
void Water13_Gold::matchRoom(int32_t placeId, ClientPtr client)
{
    bool flag = true;
    auto& rooms = Water13_Gold::roomsArr[placeId];
    for(auto& ptr : rooms)
    {
        auto w_ptr = std::static_pointer_cast<Water13_Gold>(ptr);
        if(w_ptr == nullptr)        //TODO 需要记录错误日志,理论上不会进
            continue;

        if(w_ptr->enterRoom(client))
        {
            flag = false;
            break;
        }
    }
    if(flag)
    {
        createGame(client, placeId);
    }
}

int32_t Water13_Gold::getPlayerSizeByType(int32_t placeType)
{
    int32_t sum = 0;
    auto& rooms = Water13_Gold::roomsArr[placeType];
    for(auto& ptr : rooms)
    {
        auto w_ptr = std::static_pointer_cast<Water13_Gold>(ptr);
        if(w_ptr == nullptr)        //TODO 需要记录错误日志,理论上不会进
            continue;

        sum += w_ptr->getPlayerSize();
    }

    return sum;
}

Water13_Gold::Ptr Water13_Gold::getByRoomId(RoomId roomid)
{
    return std::static_pointer_cast<Water13_Gold>(Room::get(roomid));
}

}

