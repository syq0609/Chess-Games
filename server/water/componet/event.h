/*
 * Author: LiZhaojia 
 *
 * Last modified: 2014-09-29 20:31 +0800
 *
 * Description: 通用事件订阅机制
 */

#ifndef WATER_EVENT_HPP
#define WATER_EVENT_HPP

#include <functional>
#include <memory>
#include <unordered_map>
//#include <map>
#include <vector>
#include <cstdint>

namespace water{
namespace componet{


template<typename T>
class Event;


template<typename... ParamsType>
class Event<void (ParamsType...)>
{
public:
    typedef std::function<void (ParamsType...)> Handler;
    typedef uint64_t RegID;

    enum {INVALID_REGID = 0};

    RegID reg(Handler cb)
    {
        const RegID ID = lastRegID + 1;

        const auto it = regIDs.insert(regIDs.end(), std::make_pair(ID, callbackList.size()));
        if(it == regIDs.end())
            return INVALID_REGID;

        callbackList.emplace_back(ID, cb);
        it->second = callbackList.size() - 1;

        lastRegID = ID;
        return ID;
    }

    void unreg(RegID regID)
    {
        if(callbackList.size() == 1)
        {
            callbackList.clear();
            regIDs.clear();
            return;
        }

        auto it = regIDs.find(regID);
        if(it == regIDs.end())
            return;

        //当要删的结点不在vec的尾部, 用当前的尾部覆盖要删的节点
        if(it->second + 1 != callbackList.size())
        {
            callbackList.at(it->second) = callbackList.back();
            //更新map中的index
            regIDs[callbackList.at(it->second).first] = it->second;
        }
        //删掉vec的尾部
        callbackList.pop_back();

        //删掉老的索引
        regIDs.erase(it);
    }

    // raise
    void operator()(ParamsType... args)
    {
        for(auto& cb : callbackList)
        {
            cb.second(std::forward<ParamsType>(args)...);
        }
    }

private:
    std::vector<std::pair<RegID, Handler>> callbackList;
    std::unordered_map<RegID, typename std::vector<Handler>::size_type> regIDs;
    RegID lastRegID = INVALID_REGID;
};


}}
#endif
