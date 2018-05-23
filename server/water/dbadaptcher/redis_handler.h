#ifndef PERMANENT_REDIS_HANDLER_HPP
#define PERMANENT_REDIS_HANDLER_HPP


#include "hiredis/hiredis.h"

#include "componet/class_helper.h"

#include <string>
#include <memory>
#include <unordered_map>

namespace water{
namespace dbadaptcher{


class RedisHandler
{
public:
    NON_COPYABLE(RedisHandler)
    ~RedisHandler() = default;

    void loadConfig();

public://set/get
    bool set(const std::string& key, const std::string& data);
    std::string get(const std::string& key);
    bool del(const std::string& key);

public://hash table
    bool hset(const std::string table, const std::string& key, const std::string& data);
    std::string hget(const std::string table, const std::string& key);
    bool hdel(const std::string table, const std::string& key);
    bool hgetall(std::unordered_map<std::string, std::string>* ht, const std::string table);
    //返回实际遍历的个数
    int32_t htraversal(const std::string& table, const std::function<bool (const std::string& key, const std::string& data)>& exec);

public://list
    int32_t rpush(const std::string& list, const std::string& data);
    std::string lpop(const std::string& list);

private:
    RedisHandler();
    bool init(bool force = false);

private:
    std::string m_host;
    std::size_t m_port;
    std::string m_passwd;
    std::unique_ptr<redisContext> m_ctx;

    static std::string s_cfgDir;

public:
    static void setCfgDir(const std::string& cfgDir);
    static RedisHandler& me();
};


}}

#endif
