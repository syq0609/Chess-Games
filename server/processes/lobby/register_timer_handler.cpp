/*
 * Author: LiZhaojia - waterlzj@gmail.com
 *
 * Last modified: 2017-05-19 15:10 +0800
 *
 * Description: 
 */

#include "lobby.h"
#include "room.h"
#include "client_manager.h"
#include "platform_handler.h"

#include "water/componet/logger.h"

namespace lobby{


void Lobby::registerTimerHandler()
{
    using namespace std::placeholders;
    m_timer.regEventHandler(std::chrono::seconds(1), &Room::timerExecAll);
    m_timer.regEventHandler(std::chrono::seconds(5), std::bind(&ClientManager::timerExecAll, &ClientManager::me(), _1));
    m_timer.regEventHandler(std::chrono::seconds(5), PlatformHandler::timerExec);
}


}
