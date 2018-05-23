/*
 * Description: 日志模块前端，example: 2014-07-27 09:41:23 DEBUG: hello shok
 */

#ifndef WATER_COMPONET_LOGGER_HPP
#define WATER_COMPONET_LOGGER_HPP

#include "writer.h"

#include "../class_helper.h"
#include "../spinlock.h"
#include "../format.h"

#include <functional>
#include <time.h>
#include <map>

#include <iostream>
namespace water{
namespace componet{

enum class LogLevel : uint8_t
{
    LL_ALL      = 0,
    LL_DEBUG    = 1,
    LL_TRACE    = 2,
    LL_ERROR    = 3,
};

class Logger
{
    NON_COPYABLE(Logger);
public:
    Logger(LogLevel level = LogLevel::LL_ALL);
    ~Logger();
public:
    void addWriter(std::shared_ptr<Writer> writer);
    std::shared_ptr<Writer> getWriter(const WriterType type) const;
    void eraseWriter(const WriterType type) ;
    void stop();

    template<typename... Args>   
    void trace(const Args&... args)
    {
        return this->log(LogLevel::LL_TRACE, std::forward<const Args>(args)...);
    }

    template<typename... Args>   
    void debug(const Args&... args)
    {
        return this->log(LogLevel::LL_DEBUG, std::forward<const Args>(args)...);
    }

    template<typename... Args>   
    void error(const Args&... args)
    {
        return this->log(LogLevel::LL_ERROR, std::forward<const Args>(args)...);
    }

private:
    std::string formatTime();
    const char* getLevelStr(LogLevel l);

    template<typename... Args>
    void log(LogLevel level, const std::string& formatStr, const Args&... args)
    {
        if (m_level > level)
            return;

        std::string fullFormatStr = formatTime() + " " + getLevelStr(level) + ": " + formatStr + "\n";
        const std::string text = format(fullFormatStr, args...);
        m_writerMapLock.lock(); //锁m_writerMap，防止append的时候map更改
        std::map<WriterType, std::shared_ptr<Writer>>& writerMap = m_writerMap;
        m_writerMapLock.unlock();
        for (const auto &item : writerMap)
        {
            if (item.second != nullptr)
            {
                item.second->append(text.c_str(), text.size());
            }
        }
    }

private:
    LogLevel m_level;
    std::map<WriterType, std::shared_ptr<Writer>> m_writerMap; 
    Spinlock m_writerMapLock;
};



}}

#endif //#define WATER_COMPONET_LOGGER_HPP 
