#include "platform_handler.h"

#include "gateway.h"
#include "client_manager.h"
#include "game_config.h"

#include "dbadaptcher/redis_handler.h"

#include "componet/string_kit.h"
#include "componet/logger.h"

#include "protocol/protobuf/proto_manager.h"
#include "protocol/protobuf/private/gm.codedef.h"

namespace gateway{

using water::dbadaptcher::RedisHandler;

void gmcmdHandler()
{
    const char* redislist = "pf_gw_gmcmd";
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
            if (nodes[0] == "reloadcfg")
            {
                LOG_TRACE("gmcmd from platfrom, cmd={}", nodes[0]);

                PROTO_VAR_PRIVATE(LoadGameConfig, snd);
                Gateway::me().sendToPrivate(ProcessId("lobby", 1), sndCode, snd);


                auto oldGameCfgVersion = GameConfig::me().version();
                GameConfig::me().reload();
                if (oldGameCfgVersion  != GameConfig::me().version())
                {
                    auto clientManager = Gateway::me().clientManager();
                    if (clientManager != nullptr)
                        clientManager->sendSystemNoticeToAllClients();
                }
                break;
            }
            
            LOG_ERROR("gmcmd from platfrom, unkonwn opcode, {}", nodes[0]);
        } while (false);
    }
}

void PlatformHandler::timerExec(const componet::TimePoint& now)
{
    gmcmdHandler();
}


}

