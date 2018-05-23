/*
 * Author: LiZhaojia
 *
 * Created: 2015-03-07 15:44 +0800
 *
 * Modified: 2015-03-07 15:44 +0800
 *
 * Description:  统一注册注册消息处理
 */

#include "gateway.h"

#include "water/componet/logger.h"

#include "client_manager.h"

namespace gateway{

void Gateway::registerTcpMsgHandler()
{
    using namespace std::placeholders;
 
    //clientManager
    m_clientManager->regClientMsgRelay();
    m_clientManager->regMsgHandler();
}


}
