/*
 * Author: LiZhaojia - waterlzj@gmail.com
 *
 * Last modified: 2017-06-28 18:46 +0800
 *
 * Description:  游戏配置
 */


#ifndef GATEWAY_GAME_CONFIG_H
#define GATEWAY_GAME_CONFIG_H


#include "componet/exception.h" 

#include <map>
#include <vector>
#include <set>


namespace gateway{


DEFINE_EXCEPTION(LoadGameCfgFailedGW, water::componet::ExceptionBase);

struct GameConfigData
{
    struct
    {
        std::string version;
        bool appleReview   = false;
        bool strictVersion = false;
        std::string iosAppUrl;
        std::string androidAppUrl;
    } versionInfo;

    struct
    {
        std::string wechat1;
        std::string wechat2;
        std::string wechat3;
        std::string shareLink;
    } customService;

    struct
    {
        std::string title;
        std::string content;
    } wechatShare;

    struct
    {
        std::string announcementBoard;
        std::string marquee;
    } systemNotice;

    std::map<uint32_t, int32_t> pricePerPlayer; //<房间局数, 人头费>

    struct game13RoomRuleInfo
    {
        std::string rule;
        std::string img;
        std::string version;
    };
    std::map<int32_t, game13RoomRuleInfo> game13RoomRule;

    struct
    {
        int32_t maxPlayerSize = 0;
    } roomPropertyLimits;

    struct gameInfo
    {
        bool goldMode;
        bool cardMode;
        std::string url;
        std::string version;
    };

    std::map<int32_t, gameInfo> gameList; //游戏列表
};

class GameConfig
{
public:
    void load(const std::string& cfgDir);
    void reload(const std::string& cfgDir = "");

    const GameConfigData& data() const;

    uint32_t version() const;

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

