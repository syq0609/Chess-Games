#ifndef LOBBY_ROOM_HPP
#define LOBBY_ROOM_HPP
#include "base/process_id.h"
#include "base/tcp_message.h"
#include "componet/class_helper.h"
#include "componet/datetime.h"
#include "protocol/protobuf/proto_manager.h"

#include <list>
#include <memory>
#include <unordered_map>

namespace lobby{

using namespace water;
using namespace process;


extern const char* ROOM_TABLE_NAME;

enum class GameType
{
    none = 0,
    water13,
    water13_gold,
};

using RoomId = uint32_t;

class Client;
class Room : public std::enable_shared_from_this<Room>
{
    using ClientPtr = std::shared_ptr<Client>;
public:
    TYPEDEF_PTR(Room)
    virtual ~Room();
    RoomId getId() const;
    ClientUniqueId ownerCuid();
    void setOwnerCuid(ClientUniqueId newId);

protected:
    Room(RoomId roomid, ClientUniqueId ownerCuid, GameType gameType);
    Room(ClientUniqueId ownerCuid, GameType gameType);
    void destroyLater();
private:
    void eraseFromDB() const;

public:
    virtual void clientOnlineExec(ClientPtr client) = 0;
    virtual void clientOfflineExec(ClientPtr client) = 0;
    virtual void timerExec() = 0;
    virtual void sendToAll(TcpMsgCode msgCode, const ProtoMsg& proto) = 0;
    virtual void sendToOthers(ClientUniqueId cuid, TcpMsgCode msgCode, const ProtoMsg& proto) = 0;
    virtual void giveUp(ClientPtr client) = 0;
    virtual void readyFlag(const ProtoMsgPtr& proto, ClientPtr client) = 0;
    virtual void bringOut(const ProtoMsgPtr& proto, ClientPtr client) = 0;
    virtual int32_t getPlayerSize() = 0;

private:
    RoomId m_id;
    ClientUniqueId m_ownerCuid;
    const GameType m_gameType;

public:
    static Room::Ptr get(RoomId);
    static void timerExecAll(componet::TimePoint now);
    static void clientOnline(ClientPtr client);
    static void clientOffline(ClientPtr client);

protected:
    static RoomId getRoomId();
    static bool add(Room::Ptr);
    static void delLater(Room::Ptr);
private:
    static RoomId s_lastRoomId;
    static std::list<RoomId> s_expiredIds;
    static std::unordered_map<RoomId, Room::Ptr> s_rooms;
protected:
    static componet::TimePoint s_timerTime;
};

inline RoomId Room::getId() const
{
    return m_id;
}

inline ClientUniqueId Room::ownerCuid()
{
    return m_ownerCuid;
}

inline void Room::setOwnerCuid(ClientUniqueId newId)
{
    m_ownerCuid = newId;
}

}

#endif
