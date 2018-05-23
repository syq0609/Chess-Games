/*
 * Author: LiZhaojia - waterlzj@gmail.com
 *
 * Last modified: 2017-05-19 15:10 +0800
 *
 * Description: 
 */

#include "lobby.h"

#include "client_manager.h"
#include "water13.h"
#include "water13_card.h"
#include "water13_gold.h"
#include "platform_handler.h"

#include "water/componet/logger.h"
#include "protocol/protobuf/private/gm.codedef.h"

namespace lobby{

void testPingHandler(const ProtoMsgPtr& proto, ProcessId pid)
{
//    auto msg = std::static_pointer_cast<PrivateProto::Ping>(proto);
    auto msg = PROTO_PTR_CAST_PRIVATE(Ping, proto);
    LOG_DEBUG("recv 'Ping' from {}, ping.msg = '{}'", pid, msg->msg());
}

void Lobby::registerTcpMsgHandler()
{
   using namespace std::placeholders;

   ClientManager::me().regMsgHandler();
   Water13::regMsgHandler();
   Water13_Card::regMsgHandler();
   Water13_Gold::regMsgHandler();
   PlatformHandler::regMsgHandler();


   /********************msg from client************************/
   //
   /*******************msg from cluster***********************/
   REG_PROTO_PRIVATE(Ping, std::bind(testPingHandler, _1, _2));
}


}
