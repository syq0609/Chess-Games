#ifndef GATEWAY_PLATFORM_HANDLER_H
#define GATEWAY_PLATFORM_HANDLER_H


#include "componet/datetime.h"

namespace gateway{

using namespace water;

class PlatformHandler
{
public:
    static void timerExec(const componet::TimePoint& now);
};

}


#endif
