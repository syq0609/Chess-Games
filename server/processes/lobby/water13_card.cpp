#include "water13_card.h"
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

void Water13_Card::giveUp(ClientPtr client)
{
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
                if(game->m_rounds == 0)
                {
                    if (client->cuid() == game->ownerCuid()) //房主
                    {
                        LOG_TRACE("准备期间房主离开房间, roomid={}, name={}, ccid={}, cuid={}, openid={}",
                                client->roomid(), client->name(), client->ccid(), client->cuid(), client->openid()); 
                        LOG_TRACE("准备期间终止游戏, 销毁房间, roomid={}", game->getId());
                        game->abortGame();
                        return;
                    }
                    else
                    {
                        LOG_TRACE("准备期间普通成员离开房间, roomid={}, name={},  ccid={}, cuid={}, openid={}",
                                game->getId(), client->name(), client->ccid(), client->cuid(), client->openid()); 
                        game->removePlayer(client);
                        return;
                    }
                }
                else
                {
                    game->m_startVoteTime = componet::toUnixTime(s_timerTime);
                    game->m_voteSponsorCuid = info->cuid;
                    game->m_status = GameStatus::vote;
                    //视为赞成票
                    info->vote = PublicProto::VT_AYE;
                    game->checkAllVotes();
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
        case GameStatus::settle:        //结算状态也可以投票
            { 
                game->m_startVoteTime = componet::toUnixTime(s_timerTime);
                game->m_voteSponsorCuid = info->cuid;
                game->m_status = GameStatus::vote;
            }
            //no break
        case GameStatus::vote:
            {
                //视为赞成票
                info->vote = PublicProto::VT_AYE;
                game->checkAllVotes();
            }
            break;
        //case GameStatus::settle: //结算期间, 外挂, 忽略
        //    break;
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

void Water13_Card::readyFlag(const ProtoMsgPtr& proto, ClientPtr client)
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

    if (game->m_status != GameStatus::prepare && game->m_status != GameStatus::settle)
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
    LOG_TRACE("ReadyFlag, 玩家设置准备状态, readyFlag={}, roomid={}, name={}, ccid={}, cuid={}, openid={}", rcv->ready(), client->roomid(), client->name(), client->ccid(), client->cuid(), client->openid());

    game->syncAllPlayersInfoToAllClients();

    //新的ready确认, 检查是否所有玩家都已确认, 可以启动游戏
    if (newStatus == PublicProto::S_G13_PlayersInRoom::READY)
        game->tryStartRound();
    saveToDB(game, "set ready flag, start game", client);
}

void Water13_Card::bringOut(const ProtoMsgPtr& proto, ClientPtr client)
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

    //顺利结算完毕, 开下一局        结算需要等待X秒或全准备才能进入下一轮
    //if (game->m_status == GameStatus::settle)
    //    game->tryStartRound();
}

//TODO 注册子类专属协议
void Water13_Card::regMsgHandler()
{
    using namespace std::placeholders;
    REG_PROTO_PUBLIC(C_G13_CreateGame, std::bind(&Water13_Card::proto_C_G13_CreateGame, _1, _2));
    REG_PROTO_PUBLIC(C_G13_JoinGame, std::bind(&Water13_Card::proto_C_G13_JoinGame, _1, _2));
    REG_PROTO_PUBLIC(C_G13_VoteFoAbortGame, std::bind(&Water13_Card::proto_C_G13_VoteFoAbortGame, _1, _2));
    REG_PROTO_PUBLIC(C_G13_QuickStart, std::bind(&Water13_Card::proto_C_G13_QuickStart, _1, _2));
    REG_PROTO_PUBLIC(C_G13_ReplyHalfEnter, std::bind(&Water13_Card::proto_C_G13_ReplyHalfEnter, _1, _2));
    REG_PROTO_PUBLIC(C_G13_VoteForQuickStart, std::bind(&Water13_Card::proto_C_G13_VoteForQuickStart, _1, _2));
    REG_PROTO_PUBLIC(C_G13_RenewRoom, std::bind(&Water13_Card::proto_C_G13_RenewRoom, _1, _2));
    REG_PROTO_PUBLIC(C_G13_RenewReply, std::bind(&Water13_Card::proto_C_G13_RenewReply, _1, _2));
}

void Water13_Card::proto_C_G13_CreateGame(ProtoMsgPtr proto, ClientConnectionId ccid)
{
    auto client = ClientManager::me().getByCcid(ccid);
    if (client == nullptr)
        return;

    if (client->roomid() != 0)
    {
        client->noticeMessageBox("已经在房间中, 无法创建新房间!");
        return;
    }

    auto rcv = PROTO_PTR_CAST_PUBLIC(C_G13_CreateGame, proto);
    
    int32_t playerPrice = 0;
    {//检查创建消息中的数据是否合法
        int32_t attrCheckResult = 0;
        do {
            if (rcv->play_type() !=GP_NORMAL && rcv->play_type() != GP_BANKER && rcv->play_type() != GP_SAMEONE)
            {
                attrCheckResult = 1;
                break;
            }

            const int32_t minPlayerSize = 2;
            const int32_t maxPlayerSize = 8;

            if (rcv->player_size() < minPlayerSize || rcv->player_size() > maxPlayerSize)
            {
                attrCheckResult = 2;
                break;
            }

            const auto& priceCfg = GameConfig::me().data().pricePerPlayer;
            auto priceCfgIter = priceCfg.find(rcv->rounds());
            if (priceCfgIter  == priceCfg.end())
            {
                attrCheckResult = 4;
                break;
            }

            playerPrice = priceCfgIter->second;

            if (rcv->payor() != PAY_BANKER && rcv->payor() != PAY_AA && rcv->payor() != PAY_WINNER)
            {
                attrCheckResult = 5;
                break;
            }

            if (rcv->da_qiang() != DQ_NORMAL && rcv->da_qiang() != DQ_X_DAO)
            {
                attrCheckResult = 6;
                break;
            }

            const auto& mapaiCfg = GameConfig::me().data().mapaiPattern;
            auto mapaiCfgIter = std::find(mapaiCfg.begin(), mapaiCfg.end(),rcv->ma_pai());
            if (rcv->ma_pai() > 0 && mapaiCfgIter  == mapaiCfg.end())
            {
                attrCheckResult = 7;
                break;
            }

            //if(rcv->with_ghost() != 0 && (rcv->with_ghost() < 2 || rcv->with_ghost() > 4))
            //{
            //    attrCheckResult = 8;
            //    break;
            //}

            if(rcv->suits_colors().size() > 4)
            {
                attrCheckResult = 9;
                break;
            }

            if(rcv->suits_colors().size() > 0)
            {
                if(rcv->player_size() > 4 && rcv->suits_colors().size() < rcv->player_size() - 4)
                {
                    attrCheckResult = 10;
                    break;
                }

                if(rcv->player_size() <= 4 && rcv->suits_colors().size() != 1)
                {
                    attrCheckResult = 11;
                    break;
                }

                std::vector<int32_t> allSuits = {3, 2, 1, 0};
                std::vector<int32_t> checkSuits(rcv->suits_colors().begin(), rcv->suits_colors().end());
                if(!isSubset(allSuits, checkSuits))
                {
                    attrCheckResult = 12;
                    break;
                } 
            }

        } while (false);

        //是否出错
        if (attrCheckResult > 0)
        {
            client->noticeMessageBox(componet::format("创建房间失败, 非法的房间属性设定({})!", attrCheckResult));
            return;
        }
    }

    //房间
    auto game = Water13_Card::create(client->cuid(), GameType::water13);
    if (Room::add(game) == false)
    {
        LOG_ERROR("G13, 创建房间失败, Room::add失败");
        return;
    }

    //初始化游戏信息
    auto& attr       = game->m_attr;
    attr.playType    = rcv->play_type() > GP_NORMAL ? GP_BANKER : GP_NORMAL;
    attr.payor       = rcv->payor();
    attr.daQiang    = rcv->da_qiang();
    attr.rounds      = rcv->rounds();
    attr.playerSize  = rcv->player_size();
    attr.maPai      = rcv->ma_pai();
    attr.isCrazy     = rcv->is_crazy();
    attr.halfEnter  = rcv->half_enter();
    attr.bankerMultiple  = rcv->banker_multiple();
    attr.withGhost  = rcv->with_ghost();
    attr.playerPrice = playerPrice;
    if(rcv->suits_colors().size() > 0)
    {
        attr.suitsColor.resize(rcv->suits_colors().size());
        std::copy(rcv->suits_colors().begin(), rcv->suits_colors().end(), attr.suitsColor.begin());
    }

    //玩家座位数量
    game->m_players.resize(attr.playerSize);

    //最后进房间, 因为进房间要预扣款, 进入后再有什么原因失败需要回退
    if (!game->enterRoom(client))
    {
        LOG_TRACE("建立房间失败, enterRoom 失败, name={}, cuid={}, openid={}", client->name(), client->cuid(), client->openid()); 
        return;
    }

    saveToDB(game, "create room", client);
}

void Water13_Card::proto_C_G13_JoinGame(ProtoMsgPtr proto, ClientConnectionId ccid)
{
    auto client = ClientManager::me().getByCcid(ccid);
    if (client == nullptr)
        return;

    auto rcv = PROTO_PTR_CAST_PUBLIC(C_G13_JoinGame, proto);
    auto game = Water13_Card::getByRoomId(rcv->room_id());
    if (game == nullptr)
    {
        client->noticeMessageBox("房间号不存在");
        LOG_DEBUG("申请加入房间失败, 房间号不存在, name={}, cuid={}, openid={}, roomid={}", client->name(), client->cuid(), client->openid(), rcv->room_id());
        return;
    }
    if(!game->enterRoom(client))
    {
        LOG_DEBUG("申请加入房间失败, enterRoom 失败, name={}, cuid={}, openid={}, roomid={}", client->name(), client->cuid(), client->openid(), rcv->room_id());
        return;
    }
    saveToDB(game, "join room", client);
}

void Water13_Card::proto_C_G13_VoteFoAbortGame(ProtoMsgPtr proto, ClientConnectionId ccid)
{
    auto client = ClientManager::me().getByCcid(ccid);
    if (client == nullptr)
        return;
    auto game = getByRoomId(client->roomid());
    if (game == nullptr)
    {
        LOG_ERROR("ReadyFlag, client上记录的房间号不存在, roomid={}, name={}, ccid={}, cuid={}, openid={}", 
                  client->roomid(), client->name(), client->ccid(), client->cuid(), client->openid());
        client->afterLeaveRoom(); //ERR_HANDLER
        return;
    }

    PlayerInfo* info = game->getPlayerInfoByCuid(client->cuid());
    if (info == nullptr)
    {
        LOG_ERROR("ReadyFlag, 房间中没有这个玩家的信息, roomid={}, name={}, ccid={}, cuid={}, openid={}",
                  client->roomid(), client->name(), client->ccid(), client->cuid(), client->openid());

        client->afterLeaveRoom(); //ERR_HANDLER
        return;
    }

    if (game->m_status != GameStatus::vote)
        return;

    auto rcv = PROTO_PTR_CAST_PUBLIC(C_G13_VoteFoAbortGame, proto);
    info->vote = rcv->vote();

    LOG_TRACE("voteForAbortGame, 收到投票, vote={}, roomid={}, name={}, ccid={}, cuid={}, openid={}",
              info->vote, client->roomid(), client->name(), client->ccid(), client->cuid(), client->openid());

    if (rcv->vote() == PublicProto::VT_NONE) //弃权的不处理
        return;
    

    game->checkAllVotes();
    saveToDB(game, "vote for giveup", client);
    return;
}

/*************************************************************************/
void Water13_Card::trySettleAll()
{
    //总结算
    if (m_status != GameStatus::settle)
        return;
    if (m_rounds < m_attr.rounds)
        return;
    settleAll();
}

void Water13_Card::settleAll()
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

void Water13_Card::checkAllVotes()
{
    if (m_status != GameStatus::vote)
        return;

    PROTO_VAR_PUBLIC(S_G13_AbortGameOrNot, snd);
    snd.set_sponsor(m_voteSponsorCuid);
    time_t elapse = componet::toUnixTime(s_timerTime) - m_startVoteTime;
    snd.set_remain_seconds(MAX_VOTE_DURATION > elapse ? MAX_VOTE_DURATION - elapse : 0);
    uint32_t ayeSize = 0;
    ClientUniqueId oppositionCuid = 0;
    for (PlayerInfo* info : getValidPlayers())
    {
        if (info->vote == PublicProto::VT_AYE)
        {
            ++ayeSize;
        }
        else if (info->vote == PublicProto::VT_NAY)
        {
            oppositionCuid = info->cuid;
            break;
        }
        auto voteInfo = snd.add_votes();
        voteInfo->set_vote(info->vote);
        voteInfo->set_cuid(info->cuid);
    }
    if (oppositionCuid != 0) //有反对
    {
        //结束投票, 并继续游戏
        PROTO_VAR_PUBLIC(S_G13_VoteFailed, snd1);
        snd1.set_opponent(oppositionCuid);
        for (PlayerInfo* info : getValidPlayers())
        {
            info->vote = PublicProto::VT_NONE;
            auto client = ClientManager::me().getByCuid(info->cuid);
            if (client != nullptr)
                client->sendToMe(snd1Code, snd1);
        }
        m_startVoteTime = 0;
        m_voteSponsorCuid = 0;
        m_status = GameStatus::play;
        LOG_TRACE("投票失败, 游戏继续, 反对派cuid={}, roomid={}", oppositionCuid, getId());
    }
    else if (ayeSize == getValidPlayers().size()) //全同意
    {
        //扣钱
        if (m_attr.payor == PAY_WINNER)
        {
            const int32_t price = m_attr.playerPrice * m_attr.playerSize;
            auto roomOwner = ClientManager::me().getByCuid(ownerCuid());
            if (roomOwner == nullptr)
            {//理论上不可能, 安全期间出日志
                LOG_ERROR("G13, 游戏投票终止, 赢家付费, 房主扣费, 失败, 不在线, 应扣money={}, cuid={}, openid={}", price, roomOwner->cuid(), roomOwner->openid());
            }
            else
            {
                roomOwner->addMoney(-price);
                LOG_TRACE("G13, 游戏投票终止, 赢家付费, 房主扣费, 成功, moneyChange={}, cuid={}, openid={}", -price, roomOwner->cuid(), roomOwner->openid());
            }
        }
        //结束投票, 并终止游戏的通知
        for (PlayerInfo* info : getValidPlayers())
        {
            info->vote = PublicProto::VT_NONE;
            auto client = ClientManager::me().getByCuid(info->cuid);
            if (client != nullptr)
                client->noticeMessageBox("全体通过, 游戏提前结束!");
        }
        m_startVoteTime = 0;
        m_voteSponsorCuid = 0;
        LOG_TRACE("投票成功, 游戏终止, 提前结算, roomid={}, round {}/{}", getId(), m_rounds, m_attr.rounds);
        if (m_status != GameStatus::settleAll)
            settleAll();
    }
    else
    {
        //更新投票情况, 等待继续投票
        for (PlayerInfo* info : getValidPlayers())
        {
            auto client = ClientManager::me().getByCuid(info->cuid);
            if (client != nullptr)
                client->sendToMe(sndCode, snd);
        }
    }
}

void Water13_Card::proto_C_G13_QuickStart(ProtoMsgPtr proto, ClientConnectionId ccid)
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

    auto game = getByRoomId(client->roomid());
    if (game == nullptr)
    {
        LOG_ERROR("C_G13_QuickStart, client上记录的房间号不存在, roomid={}, name={}, ccid={}, cuid={}, openid={}", 
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

    switch (game->m_status)
    {
        case GameStatus::prepare:
            {
                if (client->cuid() == game->ownerCuid()) //房主
                {
                    LOG_TRACE("房主申请提前开始, roomid={}, name={}, ccid={}, cuid={}, openid={}",
                            client->roomid(), client->name(), client->ccid(), client->cuid(), client->openid()); 

                    //检测所有玩家是否处于准备状态
                    int32_t count = 0;
                    for (PlayerInfo* info : game->getValidPlayers())
                    {
                        if(info->status != PublicProto::S_G13_PlayersInRoom::READY)
                        {
                            LOG_TRACE("有玩家未准备, 不能提前开始, roomid={}, name={}, ccid={}, cuid={}, openid={}", client->roomid(), client->name(), client->ccid(), client->cuid(), client->openid());
                            return;
                        }
                        count++;
                    }
                    if(count < 2)
                    {
                        LOG_TRACE("玩家数量不足, 不能提前开始, roomid={}, name={}, ccid={}, cuid={}, openid={}", client->roomid(), client->name(), client->ccid(), client->cuid(), client->openid());
                        return;
                    }

                    game->m_quickStartTime = componet::toUnixTime(s_timerTime);
                    game->m_status = GameStatus::quickStart;
                }
                else
                {
                    LOG_TRACE("不是房主不能申请提前开始, roomid={}, name={},  ccid={}, cuid={}, openid={}",
                            game->getId(), client->name(), client->ccid(), client->cuid(), client->openid()); 
                    return;
                }
            }
        case GameStatus::quickStart:
            {
                info->vote = PublicProto::VS_AYE;
                game->checkAllVotesQuickStart();
            }
            break;
        default:
            break;
    }
    return;
}

void Water13_Card::proto_C_G13_ReplyHalfEnter(ProtoMsgPtr proto, ClientConnectionId ccid)
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
        LOG_ERROR("AdvanceStart, client上记录的房间号不存在, roomid={}, name={}, ccid={}, cuid={}, openid={}", 
                  client->roomid(), client->name(), client->ccid(), client->cuid(), client->openid());
        client->afterLeaveRoom(); //ERR_HANDLER
        return;
    }

    auto rcv = PROTO_PTR_CAST_PUBLIC(C_G13_ReplyHalfEnter, proto);

    PlayerInfo* info = game->getPlayerInfoByCuid(rcv->cuid());
    if (info == nullptr)
    {
        return;
    }

    if(info->status != PublicProto::S_G13_PlayersInRoom::WITNESS)
    {
        return;
    }

    if (rcv->reply())
    {
        //同意,改变玩家状态为准备
        info->status = PublicProto::S_G13_PlayersInRoom::READY;
        game->syncAllPlayersInfoToAllClients();
    }
    else
    {
        Client::Ptr client = ClientManager::me().getByCuid(info->cuid);
        if (client != nullptr)
        {
            client->afterLeaveRoom();
        }
    }
}

void Water13_Card::proto_C_G13_VoteForQuickStart(ProtoMsgPtr proto, ClientConnectionId ccid)
{
    auto client = ClientManager::me().getByCcid(ccid);
    if (client == nullptr)
        return;

    auto game = getByRoomId(client->roomid());
    if (game == nullptr)
    {
        LOG_ERROR("VoteForQuickStart, client上记录的房间号不存在, roomid={}, name={}, ccid={}, cuid={}, openid={}", 
                client->roomid(), client->name(), client->ccid(), client->cuid(), client->openid());
        client->afterLeaveRoom(); //ERR_HANDLER
        return;
    }

    PlayerInfo* info = game->getPlayerInfoByCuid(client->cuid());
    if (info == nullptr)
    {
        LOG_ERROR("ReadyFlag, 房间中没有这个玩家的信息, roomid={}, name={}, ccid={}, cuid={}, openid={}",
                client->roomid(), client->name(), client->ccid(), client->cuid(), client->openid());

        client->afterLeaveRoom(); //ERR_HANDLER
        return;
    }

    if (game->m_status != GameStatus::quickStart)
        return;

    auto rcv = PROTO_PTR_CAST_PUBLIC(C_G13_VoteForQuickStart, proto);
    info->vote = rcv->vote();

    LOG_TRACE("voteForQuickStart, 收到投票, vote={}, roomid={}, name={}, ccid={}, cuid={}, openid={}",
              info->vote, client->roomid(), client->name(), client->ccid(), client->cuid(), client->openid());

    if (rcv->vote() == PublicProto::VS_NONE) //弃权的不处理
        return;
    

    game->checkAllVotesQuickStart();
    saveToDB(game, "vote for quickStart", client);
    return;
}

void Water13_Card::checkAllVotesQuickStart()
{
    if (m_status != GameStatus::quickStart)
        return;

    PROTO_VAR_PUBLIC(S_G13_QuickStartOrNot, snd);
    time_t elapse = componet::toUnixTime(s_timerTime) - m_quickStartTime;
    snd.set_remain_seconds(MAX_VOTE_QUICK_START_DURATION > elapse ? MAX_VOTE_QUICK_START_DURATION - elapse : 0);
    uint32_t ayeSize = 0;
    ClientUniqueId oppositionCuid = 0;
    for (PlayerInfo* info : getValidPlayers())
    {
        if (info->vote == PublicProto::VS_AYE)
        {
            ++ayeSize;
        }
        else if (info->vote == PublicProto::VS_NAY)
        {
            oppositionCuid = info->cuid;
            break;
        }
        auto voteInfo = snd.add_votes();
        voteInfo->set_vote(info->vote);
        voteInfo->set_cuid(info->cuid);
    }
    if (oppositionCuid != 0) //有反对
    {
        //结束投票, 并继续游戏
        PROTO_VAR_PUBLIC(S_G13_VoteStartFailed, snd1);
        snd1.set_opponent(oppositionCuid);
        for (PlayerInfo* info : getValidPlayers())
        {
            info->vote = PublicProto::VT_NONE;
            auto client = ClientManager::me().getByCuid(info->cuid);
            if (client != nullptr)
                client->sendToMe(snd1Code, snd1);
        }
        m_quickStartTime = 0;
        m_status = GameStatus::prepare;
        LOG_TRACE("投票失败, 继续等待, 反对派cuid={}, roomid={}", oppositionCuid, getId());
    }
    else if (ayeSize == getValidPlayers().size()) //全同意
    {
        m_quickStartTime = 0;
        for (PlayerInfo* info : getValidPlayers())
        {
            info->vote = PublicProto::VT_NONE;
        }
        tryStartRound();
    }
    else
    {
        //更新投票情况, 等待继续投票
        for (PlayerInfo& info : m_players)
        {
            auto client = ClientManager::me().getByCuid(info.cuid);
            if (client != nullptr)
                client->sendToMe(sndCode, snd);
        }
    }
}

void Water13_Card::proto_C_G13_RenewRoom(ProtoMsgPtr proto, ClientConnectionId ccid)
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

    auto game = getByRoomId(client->roomid());
    if (game == nullptr)
    {
        LOG_ERROR("C_G13_RenewRoom, client上记录的房间号不存在, roomid={}, name={}, ccid={}, cuid={}, openid={}", 
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

    if(game->m_newOwnerId != 0)
    {
        //已有人提前申请了
        return;
    }

    switch (game->m_status)
    {
        case GameStatus::settleAll:
            {
                auto rcv = PROTO_PTR_CAST_PUBLIC(C_G13_RenewRoom, proto);
                if(rcv->payor() != PAY_AA && rcv->payor() != PAY_BANKER)
                    return;

                game->m_newOwnerId = client->cuid();

                PROTO_VAR_PUBLIC(S_G13_RenewNotice, snd2);
                snd2.set_cuid(client->cuid());
                snd2.set_payor(rcv->payor());
                game->sendToAll(snd2Code, snd2);
            }
            break;
        default:
            break;
    }
}

void Water13_Card::proto_C_G13_RenewReply(ProtoMsgPtr proto, ClientConnectionId ccid)
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

    auto game = getByRoomId(client->roomid());
    if (game == nullptr)
    {
        LOG_ERROR("C_G13_RenewReply, client上记录的房间号不存在, roomid={}, name={}, ccid={}, cuid={}, openid={}", 
                  client->roomid(), client->name(), client->ccid(), client->cuid(), client->openid());
        client->afterLeaveRoom();
        return;
    }

    if(game->m_status != GameStatus::settleAll)
        return;

    auto rcv = PROTO_PTR_CAST_PUBLIC(C_G13_RenewReply, proto);
    if(rcv->reply())
    {
        PlayerInfo* info = game->getPlayerInfoByCuid(client->cuid());
        if (info == nullptr)
        {
            client->afterLeaveRoom();
            return;
        }
        info->renewReply = true;
        game->checkAllRenewReply();
    }
    else
    {
        //T出房间
        game->removePlayer(client);
    }
}

void Water13_Card::checkAllRenewReply()
{
    if (m_status != GameStatus::settleAll)
        return;

    //PROTO_VAR_PUBLIC(S_G13_MultipleInfo, snd);
    //time_t elapse = componet::toUnixTime(s_timerTime) - m_selMultipleTime;
    //snd.set_remain_seconds(MAX_SELECT_MULTIPLE_DURATION > elapse ? MAX_SELECT_MULTIPLE_DURATION - elapse : 0);
    uint32_t size = 0;

    //检测所有有效玩家的选择
    std::vector<PlayerInfo*> vec = getValidPlayers();
    for (PlayerInfo* info : vec)
    {
        if (info->renewReply)
        {
            ++size;
        }
    }

    if (size == vec.size() - 1) //全部选择完成      //申请者不处理
    {
        //状态切换
        for (PlayerInfo* info : vec)
        {
            info->status = PublicProto::S_G13_PlayersInRoom::PREP;
            info->rank = 0;
            //数据重置
        }
        syncAllPlayersInfoToAllClients();

        //设置新房主 并且重置所有房间以及玩家数据
        setOwnerCuid(m_newOwnerId);

        m_status = GameStatus::prepare;
        m_rounds = 0;
        //m_settleAllTime = 0;
        m_newOwnerId = 0;
    }
}

Water13_Card::Ptr Water13_Card::getByRoomId(RoomId roomid)
{
    return std::static_pointer_cast<Water13_Card>(Room::get(roomid));
}

}
