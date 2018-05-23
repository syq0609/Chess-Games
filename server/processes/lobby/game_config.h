/*
 * Author: LiZhaojia - waterlzj@gmail.com
 *
 * Last modified: 2017-06-28 18:46 +0800
 *
 * Description:  游戏配置
 */


#ifndef LOBBY_GAME_CONFIG_H
#define LOBBY_GAME_CONFIG_H


#include "componet/exception.h" 
#include "componet/datetime.h"

#include <map>
#include <vector>
#include <set>


namespace lobby{


DEFINE_EXCEPTION(LoadGameCfgFailedGW, water::componet::ExceptionBase);

struct GameConfigData
{
    struct
    {
        std::string wechat1;
        std::string wechat2;
        std::string wechat3;
    } customService;

    std::map<uint32_t, int32_t> pricePerPlayer; //<房间局数, 人头费>

//    std::set<uint32_t> allRoomSize; //所有的房间可能大小
    struct
    {
        uint32_t index = -1;
        std::vector<std::vector<uint16_t>> decks;
    } testDeck;

    struct
    {
        water::componet::TimePoint begin = water::componet::EPOCH;
        water::componet::TimePoint end   = water::componet::EPOCH;
        int32_t   awardMoney = 0;
    } shareByWeChat;

    struct
    {
        int32_t maxPlayerSize = 0;
    } roomPropertyLimits;

    struct paymentInfo
    {
        int32_t base;
        int32_t award;
    };

    struct goldShopInfo
    {
        int32_t diamond;
        int32_t money;
    };

    bool iosPaymentSwitch;
    std::map<std::string, paymentInfo> iosPayment;      //ios充值金额
    std::map<std::string, goldShopInfo> goldExchange;   //金币兑换
    std::vector<int32_t> mapaiPattern;                  //马牌配置

    int32_t shootAddPoint;          //打枪+1的数值配置

    struct water13RoomRuleInfo
    {
        int32_t score;
        int32_t min;
        int32_t max;
        int32_t status;
        int32_t playType;
        int32_t daQiang;
        int32_t playerSize;
        int32_t maPai;
        int32_t withGhost;
        int32_t bankerMultiple;
    };
    std::map<int32_t, water13RoomRuleInfo> water13RoomRule;
};

class GameConfig
{
public:
    void load(const std::string& cfgDir);
    void reload(const std::string& cfgDir = "");

    const GameConfigData& data() const;

private:
    GameConfig() = default;

    GameConfigData m_data;
    std::string m_cfgDir;
    uint32_t m_version = 0;

    static GameConfig s_me;
public:
    static GameConfig& me();
};

inline const GameConfigData& GameConfig::data() const
{
    return m_data;
}


}

#endif

