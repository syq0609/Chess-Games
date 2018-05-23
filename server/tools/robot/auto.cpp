#include "auto.h"
#include "client.h"
#include "componet/logger.h"
#include "coroutine/coroutine.h"
#include "protocol/protobuf/public/client.codedef.h"
#include <map>
#include <unistd.h>
#include "deck.h"

using namespace PublicProto;


struct Info
{
    Info ()
    {
    }

    Info (bool b_, std::string o, std::string n)
    {
        banker = b_;
        openid = o;
        name = n;
    }

    bool banker;
    std::string openid;
    std::string name;
    std::string imgurl;
    uint64_t cuid;
    std::vector<uint16_t> cards;
    int32_t roomid;
};

static Info info[2] = 
{
    {
        false,
        std::string("xxxxx1"),
        std::string("robot1"),
    },
    {
        false,
        std::string("xxxxx2"),
        std::string("robot2"),
    },

};
static Info& self = info[0];
static std::vector<Info> infoList;

std::string suitsName[] = {"♦", "♣", "♥", "♠"};
std::string ranksName[] = {"2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K", "A"};


void msgBox()
{
    while (true)
    {
        int32_t i = -1;
        auto msgbox = RECV_MSG(S_Notice, &i);
        if(infoList.size() == robotNum)
            LOG_TRACE("{} MSG_BOX: {}", i, msgbox->text());
        corot::this_corot::yield();
    }
}

void msgHandle()
{
    while (true)
    {
        int32_t i = -1;
        auto msgbox = RECV_MSG(S_Notice, &i);
        LOG_TRACE("{} MSG_BOX: {}", i, msgbox->text());
        corot::this_corot::yield();
    }
}

void msg_S_ServerVersion()
{
    while (true)
    {
        int32_t i = -1;
        auto msg = RECV_MSG(S_ServerVersion, &i);
        //LOG_TRACE("{} S_ServerVersion:", i);
        corot::this_corot::yield();
    }
}

void msg_S_G13_PlayersInRoom()
{
    while (true)
    {
        int32_t i = -1;
        auto msg = RECV_MSG(S_G13_PlayersInRoom, &i);
        //LOG_TRACE("{} S_G13_PlayersInRoom:", i);
        corot::this_corot::yield();
    }
}

void msg_S_G13_AllHands()
{
    while (true)
    {
        int32_t i = -1;
        auto msg = RECV_MSG(S_G13_AllHands, &i);
        //LOG_TRACE("{} S_G13_AllHands:", i);

        //确认准备就绪
        C_G13_ReadyFlag c_readyFlag;
        c_readyFlag.set_ready(true);
        SEND_MSG(C_G13_ReadyFlag, c_readyFlag, i);
        corot::this_corot::yield();
    }
}

void msg_S_LoginRet()
{
    while (true)
    {
        int32_t i = -1;
        auto msg = RECV_MSG(S_LoginRet, &i);
        char openid[10] = {0};
        char name[10] = {0};
        snprintf(openid, sizeof(openid), "%s%d", "xxxxx", i);
        snprintf(name, sizeof(name), "%s%d", "robot", i);

        Info info(false, openid, name);
        info.cuid = msg->cuid();
        info.roomid = msg->max_player_size();
        infoList.push_back(info);
        LOG_TRACE("login successful, <{}, {}, {}, {}, {}>", i, msg->cuid(), msg->temp_token(), infoList.size(), msg->max_player_size());

        //判断是否在房间中
        if(msg->max_player_size() > 0)
        {
            //C_G13_ReadyFlag c_readyFlag;
            //c_readyFlag.set_ready(true);
            //SEND_MSG(C_G13_ReadyFlag, c_readyFlag, i);
            //C_G13_GiveUp c_giveUp;
            //SEND_MSG(C_G13_GiveUp, c_giveUp, i);
        }
        else
        {
            //进入金币场填充位置(游戏服添加逻辑, 机器人不能产生新房间)
            C_G13Gold_QuickStart c_g_quickStart;
            SEND_MSG(C_G13Gold_QuickStart, c_g_quickStart, i);
        }
        corot::this_corot::yield();
    }
}

void msg_S_G13_RoomAttr()
{
    while (true)
    {
        int32_t i = -1;
        auto msg = RECV_MSG(S_G13_RoomAttr, &i);
        LOG_TRACE("robot {} enter room successful, <{}>", i, msg->room_id());

        //确认准备就绪
        C_G13_ReadyFlag c_readyFlag;
        c_readyFlag.set_ready(true);
        SEND_MSG(C_G13_ReadyFlag, c_readyFlag, i);
        corot::this_corot::yield();
    }
}

void msg_S_G13_PlayerQuited()
{
    while (true)
    {
        int32_t i = -1;
        auto msg = RECV_MSG(S_G13_PlayerQuited, &i);
        LOG_TRACE("robot {} Quited, <{}>", i);

        infoList[i].roomid = 0;
        //判断是否有钱,重新进入房间
        //身上金钱不足以进入需要进入的场次

        //重新进入房间
        C_G13Gold_QuickStart c_g_quickStart;
        SEND_MSG(C_G13Gold_QuickStart, c_g_quickStart, i);
        corot::this_corot::yield();
    }
}

std::string cardName(uint16_t c)
{
    if(c == 53)
        return "☺X";
    if(c == 54)
        return "☻X";
    return suitsName[(c - 1) /13] + ranksName[(c - 1) % 13];
}

void msg_S_G13_HandOfMine()
{
    while (true)
    {
        int32_t j = -1;
        auto msg = RECV_MSG(S_G13_HandOfMine, &j);
        std::string cards;
        std::array<Deck::Card, 13> tmp;
        std::copy(msg->cards().begin(), msg->cards().end(), tmp.begin());
        for (auto i = 0u; i < tmp.size(); ++i)
        {
            cards.append(std::to_string(tmp[i]));
            cards.append(", ");
        }
        LOG_TRACE("handOfMine robot {}, [{}], count {}", j, cards, tmp.size());
        int rounds = msg->rounds();
        if(rounds != 1)
        {
            LOG_ERROR("gold room rounds != 1 {}", rounds);
        }
        self.cards.clear();
        for(auto j = 0u; j < tmp.size(); ++j)
        {
            self.cards.push_back(tmp[j]);
        }
        LOG_TRACE("get cards successful, <{}>",self.cards.size());

        if(self.cards.size() == 8)
        {
            //抢庄模式, 金币房暂不考虑 
            //LOG_TRACE("error: bankerMode is not open");
        }

        if(self.cards.size() == 13)
        {
            //理牌
            Deck deck;
            std::vector<uint16_t> retCards;

            clock_t start,ends;
            start=clock();
            deck.getRecommendPokerSet(self.cards, retCards);
            ends=clock();
            printf("---------------------------------------- %ld\n", (ends - start) / 1000);

            //出牌
            C_G13_BringOut c_bringOut;
            for(int k = 0; k < 13; ++k)
            {
                c_bringOut.add_cards(retCards[k]);
            }
            c_bringOut.set_special(false);
            SEND_MSG(C_G13_BringOut, c_bringOut, j);
            //LOG_TRACE("robot {} bringout successful", j);
            LOG_TRACE("robot {} bringout successful, <{}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}, {}>", j, cardName(retCards[0]), cardName(retCards[1]), cardName(retCards[2]), cardName(retCards[3]), cardName(retCards[4]), cardName(retCards[5]), cardName(retCards[6]), cardName(retCards[7]), cardName(retCards[8]), cardName(retCards[9]), cardName(retCards[10]), cardName(retCards[11]), cardName(retCards[12]));
        }
        corot::this_corot::yield();
    }
}

void msg_S_Chat()
{
    while (true)
    {
        int32_t i = -1;
        auto msg = RECV_MSG(S_Chat, &i);
        const auto& ctn = msg->content();
        switch (ctn.type())
        {
            case CHAT_TEXT:
                LOG_TRACE("player talk, cuid={}, text={}", msg->cuid(), ctn.data_text());
                break;
            case CHAT_FACE:
                LOG_TRACE("player emijo, cuid={}, code={}", msg->cuid(), ctn.data_int());
                break;
            case CHAT_VOICE:
                LOG_TRACE("player voice, cuid={}, code={}", msg->cuid(), ctn.data_int());
                break;
        }
    }
}

void AutoActions::init()
{
    corot::create(msg_S_ServerVersion);
    corot::create(std::bind(&AutoActions::start, this));
    corot::create(msgBox);
    corot::create(msg_S_G13_PlayersInRoom);
    corot::create(msg_S_G13_HandOfMine);
    corot::create(msg_S_Chat);
    corot::create(msg_S_LoginRet);
    corot::create(msg_S_G13_RoomAttr);
    corot::create(msg_S_G13_AllHands);
    //corot::create(msgHandle);
}


void AutoActions::start()
{
    int32_t useCount = 13;
    C_Login c_login;
    c_login.set_login_type(LOGINT_VISTOR);
    c_login.set_token("OOOOOO");
    c_login.set_imgurl("");
    c_login.set_sex(1);
    c_login.set_os("");
    char openid[10] = {0};
    char name[10] = {0};

    //定时检测,没有进入房间的机器人,请求进入房间

    for(int i = 0; i < useCount; ++i)
    {
        memset(openid, 0, sizeof(openid));
        memset(name, 0, sizeof(name));
        snprintf(openid, sizeof(openid), "%s%d", "xxxxx", i);
        snprintf(name, sizeof(name), "%s%d", "robot", i);

        c_login.set_openid(openid);
        c_login.set_nick_name(name);
        SEND_MSG(C_Login, c_login, i);

        //C_G13_GiveUp c_giveUp;
        //SEND_MSG(C_G13_GiveUp, c_giveUp, i);
    }

    LOG_TRACE("All robots are logged in, <{}>", infoList.size());

    //for(uint32_t i = 0; i < infoList.size(); ++i)
    //{
    //    if(infoList[i].roomid > 0)
    //    {
    //        // TODO
    //        LOG_TRACE("robot {} in room, <{}>", i, infoList[i].roomid);

    //        C_G13_GiveUp c_giveUp;
    //        SEND_MSG(C_G13_GiveUp, c_giveUp, i);

    //        //auto playerQuite = RECV_MSG(S_G13_PlayerQuited, i);
    //        //LOG_TRACE("RECVED, S_G13_PlayerQuited, me.cuid={}", self.cuid);
    //    }
    //    else
    //    {
    //        LOG_TRACE("robot {} not in room, <{}>", i, infoList[i].roomid);
    //        //进入金币场填充位置(游戏服添加逻辑, 机器人不能产生新房间)
    //        C_G13Gold_QuickStart c_g_quickStart;
    //        SEND_MSG(C_G13Gold_QuickStart, c_g_quickStart, i);

    //        //获取房间信息
    //        //auto attr = RECV_MSG(S_G13_RoomAttr, i);
    //        //LOG_TRACE("{} enter room successful, <{}, {}>", i, attr->room_id(), attr->banker_cuid());

    //        ////确认准备就绪
    //        //C_G13_ReadyFlag c_readyFlag;
    //        //c_readyFlag.set_ready(true);
    //        //SEND_MSG(C_G13_ReadyFlag, c_readyFlag, i);
    //    }
    //}


    ////说话
    //C_SendChat c_chat;
    //c_chat.set_type(CHAT_TEXT);
    //c_chat.set_data_text("hello, every one!");
    //SEND_MSG(C_SendChat, c_chat);
}

std::string AutoActions::cardName(uint16_t c)
{
    if(c == 53)
        return "☺X";
    if(c == 54)
        return "☻X";
    return suitsName[(c - 1) /13] + ranksName[(c - 1) % 13];
}

