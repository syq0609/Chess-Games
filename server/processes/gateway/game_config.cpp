#include "game_config.h"

#include "componet/logger.h"
#include "componet/xmlparse.h"
#include "componet/string_kit.h"


namespace gateway{

using namespace water;

GameConfig GameConfig::s_me;
GameConfig& GameConfig::me()
{
    return s_me;
}

uint32_t GameConfig::version() const
{
    return m_version;
}

void GameConfig::reload(const std::string& cfgDir /* = "" */)
{
    const std::string dir = (cfgDir != "") ? cfgDir : m_cfgDir;
    try
    {
        load(dir);
    }
    catch (const LoadGameCfgFailedGW& ex)
    {
        const std::string configFile = dir + "/game.xml";
        LOG_ERROR("reload {}, failed! {}", configFile, ex);
    }
}

void GameConfig::load(const std::string& cfgDir)
{
    using componet::XmlParseDoc;
    using componet::XmlParseNode;

    const std::string configFile = cfgDir + "/game.xml";
    LOG_TRACE("load game config, file={}", configFile);

    XmlParseDoc doc(configFile);
    XmlParseNode root = doc.getRoot();
    if (!root)
        EXCEPTION(LoadGameCfgFailedGW, configFile + "prase root node failed");

    uint32_t version = root.getAttr<uint32_t>("version");
    if (version <= m_version)
    {
        m_cfgDir = cfgDir;
        LOG_TRACE("load {}, ignored, version:{}->{}", configFile, m_version, version);
        return;
    }

    GameConfigData data;
    {
        XmlParseNode versionNode = root.getChild("version");
        if (!versionNode)
            EXCEPTION(LoadGameCfgFailedGW, "version node dose not exist");

        data.versionInfo.version = versionNode.getAttr<std::string>("version");
        data.versionInfo.appleReview   = versionNode.getAttr<bool>("appleReview");
        data.versionInfo.strictVersion = versionNode.getAttr<bool>("strictVersion");
        data.versionInfo.iosAppUrl     = versionNode.getAttr<std::string>("iosAppUrl");
        data.versionInfo.androidAppUrl = versionNode.getAttr<std::string>("androidAppUrl");
    }

    {
        XmlParseNode customServiceNode = root.getChild("customService");
        if (!customServiceNode)
            EXCEPTION(LoadGameCfgFailedGW, "customService node dose not exist");
        data.customService.wechat1   = customServiceNode.getAttr<std::string>("wechat1");
        data.customService.wechat2   = customServiceNode.getAttr<std::string>("wechat2");
        data.customService.wechat3   = customServiceNode.getAttr<std::string>("wechat3");
        data.customService.shareLink = customServiceNode.getAttr<std::string>("shareLink");
    }

    {
        XmlParseNode systemNoticeNode = root.getChild("systemNotice");
        if (!systemNoticeNode)
            EXCEPTION(LoadGameCfgFailedGW, "systemNotice node dose not exist");

        XmlParseNode announcementBoardNode = systemNoticeNode.getChild("announcementBoard");
        if (!announcementBoardNode)
            EXCEPTION(LoadGameCfgFailedGW, "systemNotice.announcementBoard node dose not exist");
        data.systemNotice.announcementBoard = componet::format("{}$$${}$$${}$$${}", 
                                                     announcementBoardNode.getAttr<std::string>("salution"),
                                                     announcementBoardNode.getAttr<std::string>("body"),
                                                     announcementBoardNode.getAttr<std::string>("signature"),
                                                     announcementBoardNode.getAttr<std::string>("date"));


        XmlParseNode marqueeNode = systemNoticeNode.getChild("marquee");
        if (!marqueeNode)
            EXCEPTION(LoadGameCfgFailedGW, "systemNotice.marquee node dose not exist");
        data.systemNotice.marquee = marqueeNode.getAttr<std::string>("text");

    }

    {
        XmlParseNode pricePerPlayerNode = root.getChild("pricePerPlayer");
        if (!pricePerPlayerNode)
            EXCEPTION(LoadGameCfgFailedGW, "pricePerPlayer node dose not exist");

        data.pricePerPlayer.clear();
        for (XmlParseNode itemNode = pricePerPlayerNode.getChild("item"); itemNode; ++itemNode)
        {
            uint32_t rounds = itemNode.getAttr<uint32_t>("rounds");
            int32_t  money  = itemNode.getAttr<int32_t>("money");
            data.pricePerPlayer[rounds] = money;
        }

    }

    {
        XmlParseNode roomPropertyLimitsNode = root.getChild("roomPropertyLimits");
        if (!roomPropertyLimitsNode)
            EXCEPTION(LoadGameCfgFailedGW, "roomPropertyLimits node dose not exist");
        data.roomPropertyLimits.maxPlayerSize = roomPropertyLimitsNode.getAttr<int32_t>("maxPlayerSize");
    }

    {
        XmlParseNode gameListNode = root.getChild("gameList");
        if (!gameListNode)
            EXCEPTION(LoadGameCfgFailedGW, "gameList node dose not exist");

        data.gameList.clear();
        for (XmlParseNode itemNode = gameListNode.getChild("item"); itemNode; ++itemNode)
        {
            int32_t type = itemNode.getAttr<int32_t>("gameid");
            GameConfigData::gameInfo gInfo;
            gInfo.goldMode = itemNode.getAttr<bool>("goldMode");
            gInfo.cardMode = itemNode.getAttr<bool>("cardMode");
            gInfo.url = itemNode.getAttr<std::string>("url");
            gInfo.version = itemNode.getAttr<std::string>("version");
            data.gameList[type] = gInfo;
        }
    }
    {
        XmlParseNode wechatShareNode = root.getChild("wechatShare");
        if (!wechatShareNode)
            EXCEPTION(LoadGameCfgFailedGW, "wechatShare node dose not exist");
        data.wechatShare.title   = wechatShareNode.getAttr<std::string>("title");
        data.wechatShare.content   = wechatShareNode.getAttr<std::string>("content");
    }

    m_data    = data;
    m_cfgDir  = cfgDir;

    LOG_TRACE("load {}, successed, version:{}->{}", configFile, m_version, version);
    m_version = version;
}

}


