#include "platform_handler.h"

#include "client_manager.h"
#include "game_config.h"
#include "water13.h"

#include "dbadaptcher/redis_handler.h"

#include "componet/string_kit.h"
#include "componet/logger.h"

#include "protocol/protobuf/private/gm.codedef.h"


namespace lobby{

using water::dbadaptcher::RedisHandler;

void rechargeHandler()
{
    const char* redislist = "pf_lb_recharge";
    auto& redis = RedisHandler::me();
    for( std::string msg = redis.lpop(redislist); msg != ""; msg = redis.lpop(redislist) )
    {
        LOG_TRACE("recharge msg from platfrom, {}", msg);
        std::vector<std::string> nodes = componet::splitString(msg, ",");
        if (nodes.size() < 4)
        {
            LOG_ERROR("recharge msg from platfrom, drop broken msg, itemsize={}", nodes.size());
            continue;
        }
        const auto sn    = componet::fromString<int32_t>(nodes[0]);
        const auto cuid  = componet::fromString<ClientUniqueId>(nodes[1]);
        const auto money = componet::fromString<int32_t>(nodes[2]);

        ClientManager::me().rechargeMoney(sn, cuid, money, nodes[3]);
    }
}

void gmcmdHandler()
{
    const char* redislist = "pf_lb_gmcmd";
    auto& redis = RedisHandler::me();
    for( std::string msg = redis.lpop(redislist); msg != ""; msg = redis.lpop(redislist) )
    {   
        LOG_TRACE("gmcmd from platfrom, {}", msg);
        std::vector<std::string> nodes = componet::splitString(msg, ",");
        if (nodes.size() < 1)
        {   
            LOG_ERROR("gmcmd from platfrom, drop broken msg, itemsize={}", nodes.size());
            continue;
        }   
        do {
            if (nodes[0] == "dismissroom")
            {
                LOG_TRACE("gmcmd from platfrom, cmd={}", nodes[0]);
                std::vector<std::string> nodes = componet::splitString(msg, ",");
                if (nodes.size() < 3)
                {
                    LOG_ERROR("dismiss msg from platfrom, drop broken msg, itemsize={}", nodes.size());
                    continue;
                }
                const auto roomid    = componet::fromString<RoomId>(nodes[1]);
                const auto ownerCuid = componet::fromString<ClientUniqueId>(nodes[2]);

                auto room = Room::get(roomid);
                if (room == nullptr)
                {
                    LOG_TRACE("gm dismiss room, roomid={}, ownercuid={}, not exisit", roomid, ownerCuid);
                    continue;
                }

                if (room->ownerCuid() != ownerCuid)
                {
                    LOG_TRACE("gm dismiss room, roomid={}, ownercuid={}, ownerCuid not matched, realOwnerCuid={}", roomid, ownerCuid, room->ownerCuid());
                    continue;
                }

                LOG_TRACE("gm dismiss room, roomid={}, ownercuid={}, successed", roomid, ownerCuid, room->ownerCuid());
                auto game = std::static_pointer_cast<Water13>(room);
                game->abortGame("GM 强制解散了房间");
                break;
            }
            LOG_ERROR("gmcmd from platfrom, unkonwn opcode, {}", nodes[0]);
        } while (false);
    }   
}

void PlatformHandler::timerExec(const componet::TimePoint& now)
{
    rechargeHandler();
    gmcmdHandler();
}


/*********************************************************************/


void PlatformHandler::regMsgHandler()
{
    using namespace std::placeholders;

//    auto loadGameCfg = [](ClientConnectionId ccid) { GameConfig::me().reload(); };
    REG_PROTO_PRIVATE(LoadGameConfig, std::bind( ([](ClientConnectionId ccid) { GameConfig::me().reload();}), _2 ));
}

}

