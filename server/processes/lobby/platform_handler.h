#ifndef LOBBY_PLATFORM_HANDLER_H
#define LOBBY_PLATFORM_HANDLER_H


#include "componet/datetime.h"

namespace lobby{

using namespace water;

class PlatformHandler
{
public:
    static void timerExec(const componet::TimePoint& now);

public:
    static void regMsgHandler();   
};

}


#endif
