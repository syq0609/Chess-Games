/*
 * Author: LiZhaojia
 *
 * Created: 2015-03-07 15:44 +0800
 *
 * Modified: 2015-03-07 15:44 +0800
 *
 * Description:  统一注册注册定时器事件
 *              定时器回调原型: std::function<void (const componet::TimePoint&)>;
 */

#include "gateway.h"

#include "client_manager.h"
#include "platform_handler.h"

#include "water/componet/logger.h"


namespace gateway{



void Gateway::registerTimerHandler()
{
    using namespace std::placeholders;
    //m_timer.regEventHandler(std::chrono::seconds(10), std::bind(Test::timerExec, _1));
    m_timer.regEventHandler(std::chrono::milliseconds(10), std::bind(&ClientManager::timerExec, m_clientManager, _1));
    m_timer.regEventHandler(std::chrono::seconds(5), PlatformHandler::timerExec);
}


}
