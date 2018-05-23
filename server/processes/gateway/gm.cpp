#include "gm.h"

#include "componet/format.h"
#include "componet/logger.h"

#include "protocol/protobuf/proto_manager.h"
#include "protocol/protobuf/public/client.codedef.h"
#include "protocol/protobuf/private/gm.codedef.h"
namespace gateway{

using namespace water;


void Test::timerExec(const componet::TimePoint& now)
{
    PrivateProto::Ping ping;
    ping.set_msg(componet::format("hello, I am {}", Gateway::me().getId()));
    Gateway::me().sendToPrivate(ProcessId("lobby", 1), PROTO_CODE_PRIVATE(Ping), ping);
    LOG_DEBUG("send proto msg Ping to {}", Gateway::me().getId());
}

}
