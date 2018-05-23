/*
 * Author: LiZhaojia 
 *
 * Last modified: 2014-11-24 11:18 +0800
 *
 * Description: 定时器
 */

#include "datetime.h"
#include "event.h"
#include "spinlock.h"

#include <map>
#include <set>
#include <mutex>
#include <chrono>
#include <functional>

namespace water{
namespace componet{

class Timer
{
    using TheClock = std::chrono::steady_clock;
    using TheTimePoint = TheClock::time_point;
public:
    typedef std::pair<TheClock::duration, Event<void (const TimePoint&)>::RegID> RegID;

    Timer();
    ~Timer() = default;

    int64_t precision() const;

    void operator()();

    //注册一个触发间隔
    RegID regEventHandler(std::chrono::milliseconds interval,
                         const std::function<void (const TimePoint&)>& handle);
    void unregEventHandler(RegID);
public:
    Event<void (Timer*)> e_stop;
private:
    void cleanEventHandler();
private:
    struct EventHandlers
    {
        Event<void (const TimePoint&)> event;
        TheClock::time_point regTime;
        uint64_t counter;
    };

    Spinlock m_handlersLock;
    std::map<TheClock::duration, EventHandlers> m_eventHandlers;
    Spinlock m_unregLock;
    std::set<RegID> m_unregedIds;
};

}}
