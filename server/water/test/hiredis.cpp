#include "hiredis/hiredis.h"



#define LOG_ERROR(text) std::cout << (text) << std::endl;

class RedisHandler
{
public:
    RedisHandler();
    ~RedisHandler() = default;

    bool saveData(const std::string& key, const std::string& data);
    std::string loadData(const std::string& key);

    bool saveToHashtable(const std::string table, const std::string& key, const std::string& data);
    std::string loadFromHashtable(const std::string table, const std::string& key);
    uint32_t traversalHashtable(const std::string& tablen, std::function<bool (const std::string& key, const std::string& data)>);

private:
    void loadConfig();

private:
    std::string m_redisServerHost;
    std::size_t m_port;
    std::unique_ptr<redisContext> m_ctx;
};


#define makeReply(ret) (std::shared_ptr<redisReply>(ret, freeReplyObject))


RedisHandler::RedisHandler()
    :m_redisServerHost("127.0.0.1"), m_port(6379)
{

}

bool RedisHandler::init()
{
    if (m_ctx == nullptr)
        m_ctx.reset(redisConnect(m_redisServerHost.c_str(), m_port));

    if (m_ctx == nullptr || c->err)
    {
        cout << ("connection to redis-server failed") << endl;
        return false;
    }
    return true;
}

bool set(const std::string& key, const std::string& data)
{
    if (!init())
        return false;

    const std::string& cmd = componet::format("set {} {}");
    void* ret = redisCommand(m_ctx, "%b", cmd.data(), cmd.size());
    if (ret == nullptr)
        return false;

    auto replay = makeReply(ret);
    return (replay->type == REDIS_REPLY_STATUS) && (strcasecmp(ret->str, "OK") != 0)
}

std::string loadData(const std::string& key)
{
    if (!init())
        return "";


    const std::string& cmd = componet::format("get {}", key);
    void* ret = redisCommand(m_ctx, "%b", cmd.data(), cmd.size());
    if (ret == nullptr)
        return false;

    auto replay = makeReply(ret);
    if (r->type != REDIS_REPLY_STRING)
        return false;

    return std::string(replay->str, replay->len);
}

bool saveToHashtable(const std::string table, const std::string& key, const std::string& data)
{
    if (!init())
        return false;

    return true;
}

std::unordered_map<std::string, std::string> loadFromHashtable(const std::string table, const std::string& key)
{
    std::unordered_map<std::string, std::string> ret;
    if (!init())
        return "";
    return "";
}

uint32_t traversalHashtable(const std::string& tablen, std::function<bool (const std::string& key, const std::string& data)>)
{
    if (!init())
        return 0;
}


