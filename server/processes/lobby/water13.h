#ifndef LOBBY_WATER13_SUP_HPP
#define LOBBY_WATER13_SUP_HPP

#include "room.h"

#include "protocol/protobuf/proto_manager.h"

#include "deck.h"

namespace lobby{

class Client;

class Water13 : public Room
{
    const time_t MAX_VOTE_DURATION = 180;               //投票退出(房卡特有)
    const time_t MAX_VOTE_QUICK_START_DURATION = 300;   //快速开始(房卡特有)
    const time_t MAX_SELECT_BANKER_DURATION = 300;      //选择庄家
    const time_t MAX_SELECT_MULTIPLE_DURATION = 300;    //闲家点数
    const time_t MAX_WAIT_NEXT_DURATION = 120;          //回合休息时间(房卡与金币配置值可能不一样)
    const time_t MAX_WAIT_OVER_DURATION = 180;          //销毁房间等待时间(等待续局时间)(房卡特有)

    friend class Water13_Card;
    friend class Water13_Gold;
    using ClientPtr = std::shared_ptr<Client>;

    enum class GameStatus
    {
        prepare     = 0,    //建房之后
        quickStart  = 1,    //申请提前开始状态          //房卡特有
        selBanker   = 2,    //抢庄
        selMultiple = 3,    //选择倍率
        play        = 4,    //发牌之后
        vote        = 5,    //投票
        settle      = 6,    //一局结束
        settleAll   = 7,    //总结算
        closed      = 8,    //所有局结束
    };

    enum
    {
        GP_NORMAL   = 0,        //玩法: 普通
        GP_BANKER   = 1,        //玩法: 庄家
        GP_SAMEONE  = 2,        //玩法: 全一色

        PAY_BANKER  = 0,        //支付: 庄家
        PAY_AA      = 1,        //支付: 均摊
        PAY_WINNER  = 2,        //支付: 赢家
        PAY_FREE    = 3,        //支付: 免费(用于金币场自动创建)

        DQ_NORMAL   = 0,        //打枪: 默认(翻倍)
        DQ_X_DAO    = 1,        //打枪: +1 (最好通过配置来设置这个值)
    };

public:
    void abortGame();
    void abortGame(const std::string& noticeToClient);

private:
    CREATE_FUN_NEW(Water13);
    TYPEDEF_PTR(Water13);
    virtual ~Water13();

    using Room::Room;
    
    bool enterRoom(ClientPtr client);

    void tryStartRound();
    virtual void trySettleGame();

    void trySettleAll();
    void settleAll();
    void removePlayer(ClientPtr client);

    virtual void afterEnterRoom(ClientPtr client);

    virtual void checkAllSelectMultiple();
    virtual void checkAllChipInPoint();
    virtual int32_t getPlayerSize();

    virtual void giveUp(ClientPtr client){};
    virtual void readyFlag(const ProtoMsgPtr& proto, ClientPtr client){};
    virtual void bringOut(const ProtoMsgPtr& proto, ClientPtr client){};

    virtual void sendToAll(TcpMsgCode msgCode, const ProtoMsg& proto) override;
    virtual void sendToOthers(ClientUniqueId cuid, TcpMsgCode msgCode, const ProtoMsg& proto) override;
    virtual void timerExec() override;
    virtual void clientOnlineExec(ClientPtr) override;
    virtual void clientOfflineExec(ClientPtr client) override;

    void syncAllPlayersInfoToAllClients(); //这个有空可以拆成 sendAllToMe和sendMeToAll, 现在懒得搞了
    struct RoundSettleData;
    template<typename Player>
    static std::shared_ptr<RoundSettleData> calcRound(const std::vector<Player>& players, int32_t daQiang, int32_t maPai, bool quanLeiDa = true);

    const uint32_t NO_POS = -1;
    uint32_t getEmptySeatIndex();
    struct PlayerInfo;
    PlayerInfo* getPlayerInfoByCuid(ClientUniqueId cuid);
    std::vector<PlayerInfo*> getValidPlayers();

    std::string serialize();

private:
    Room::Ptr m_room;

    struct //游戏的基本属性, 建立游戏时确定, 游戏过程中不变
    {
        int32_t playType   = GP_NORMAL;         //玩法
        int32_t payor      = PAY_BANKER;        //支付方式
        int32_t daQiang    = DQ_NORMAL;         //打枪
        int32_t rounds     = 0;                 //局数
        int32_t playerSize = 0;                 //人数
        int32_t maPai      = 0;                 //马牌
        bool    isCrazy    = false;             //疯狂场
        bool    halfEnter  = false;             //中场可进
        bool    withGhost  = false;             //带鬼
        int32_t bankerMultiple = 0;             //抢庄最高倍率
        int32_t playerPrice = 0;
        std::vector<int32_t> suitsColor;        //多色
        bool    goldPlace  = false;             //是否金币场
        int32_t placeType  = 0;                 //所属金币场次
    } m_attr;

    struct PlayerInfo
    {
        PlayerInfo(ClientUniqueId cuid_ = 0, std::string name_ = "", int32_t status_ = 0) //, int32_t money_ = 0)
        :cuid(cuid_), lastCuid(cuid_), name(name_), status(status_)//, money(money_)
        {
        }
        void clear()
        {
            cuid = 0;
            status = 0;
        }
        ClientUniqueId cuid;
        ClientUniqueId lastCuid;
        std::string name;
        std::string imgurl;
        std::string ipstr;
        int32_t status;
        int32_t vote = 0;           //提前开始 与 申请结束(投票选择项)
        int32_t rank = 0;           //当前为止, 本场比赛中累计得到的分数
        bool cardsSpecBrand = false;
        std::array<Deck::Card, 13> cards;
        int32_t chipIn = 0;             //下注点数(抢庄)
        int32_t multiple = 0;           //下注点数(比牌)
        bool renewReply = false;        //是否同意续局
        bool online = false;
    };
    std::vector<PlayerInfo> m_players;

    //游戏动态属性
    GameStatus m_status = GameStatus::prepare;
    int32_t m_rounds = 0;
    time_t m_startVoteTime = 0;
    ClientUniqueId m_voteSponsorCuid = 0;
    time_t m_settleAllTime = 0;             //等待房间销毁时间(申请续费可申请时间)
    time_t m_quickStartTime = 0;            //申请快速开始时间
    time_t m_selBankerTime = 0;             //抢庄开始时间
    ClientUniqueId m_bankerCuid = 0;        //抢庄,庄家id
    time_t m_selMultipleTime = 0;           //选择倍率开始时间
    time_t m_settleTime = 0;                //一局结束后的等待时间
    ClientUniqueId m_newOwnerId = 0;        //申请续费的玩家id
    std::vector<uint16_t> remainBrands;     //抢庄模式,剩余没发的牌

    //每轮比牌结果
    struct RoundSettleData
    {
        TYPEDEF_PTR(RoundSettleData)
        CREATE_FUN_MAKE(RoundSettleData)

        struct PlayerData
        {
            ClientUniqueId cuid;
            std::array<Deck::Card, 13> cards;   //所有牌
            std::array<Deck::BrandInfo, 3> dun; //3墩牌型
            Deck::G13SpecialBrand spec = Deck::G13SpecialBrand::none;         //特殊牌型
            std::array<int32_t, 3> dun_prize;  //每墩基础分值
            int32_t prize = 0;
            std::map<uint32_t, std::array<int32_t, 2>> losers; //<loserIndex, <price, 打枪>>
            bool quanLeiDa = false;
            bool hasMaPai = false;
        };
        std::vector<PlayerData> players;
    };
    std::vector<RoundSettleData::Ptr> m_settleData;

private://消息处理
    static void proto_C_G13_GiveUp(ProtoMsgPtr proto, ClientConnectionId ccid);
    static void proto_C_G13_ReadyFlag(ProtoMsgPtr proto, ClientConnectionId ccid);
    static void proto_C_G13_BringOut(ProtoMsgPtr proto, ClientConnectionId ccid);
    //static void proto_C_G13_SimulationRound(ProtoMsgPtr proto, ClientConnectionId ccid);
    static void proto_C_G13_ChipInPoint(ProtoMsgPtr proto, ClientConnectionId ccid);
    static void proto_C_G13_SelectMultiple(ProtoMsgPtr proto, ClientConnectionId ccid);

public:
    static Water13::Ptr getByRoomId(RoomId roomId);
    static void regMsgHandler();

    static bool recoveryFromDB();
    static bool saveToDB(Water13::Ptr game, const std::string& log, ClientPtr client);

    static bool hasMaPai(Deck::Card* c, int32_t size, int32_t maPai);
    static bool isSubset(std::vector<int32_t> v1, std::vector<int32_t> v2);
private:
    static Water13::Ptr deserialize(const std::string& bin);
    static std::string serialize(Water13::Ptr obj);

    static Deck s_deck;
};

}

#endif
