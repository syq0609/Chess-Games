#ifndef LOBBY_WATER13_GOLD_HPP
#define LOBBY_WATER13_GOLD_HPP



#include "room.h"

#include "protocol/protobuf/proto_manager.h"

#include "deck.h"
#include "water13.h"

namespace lobby{

class Client;

class Water13_Gold : public Water13
{
    friend class Water13;
    using ClientPtr = std::shared_ptr<Client>;

private:
    CREATE_FUN_NEW(Water13_Gold);
    TYPEDEF_PTR(Water13_Gold);
    using Water13::Water13;

    virtual void giveUp(ClientPtr client);
    virtual void readyFlag(const ProtoMsgPtr& proto, ClientPtr client);
    virtual void bringOut(const ProtoMsgPtr& proto, ClientPtr client);

private://消息处理      //金币场特有协议
    static void proto_C_G13Gold_GetOpenPlace(ProtoMsgPtr proto, ClientConnectionId ccid);
    static void proto_C_G13_EnterGoldPlace(ProtoMsgPtr proto, ClientConnectionId ccid);
    static void proto_C_G13Gold_ChangeRoom(ProtoMsgPtr proto, ClientConnectionId ccid);
    static void proto_C_G13Gold_QuickStart(ProtoMsgPtr proto, ClientConnectionId ccid);

    //特有函数
    static void createGame(ClientPtr client, int32_t placeId);
    static void matchRoom(int32_t placeId, ClientPtr client);
    static int32_t getPlayerSizeByType(int32_t placeType);

public:
    static void regMsgHandler();
    static Ptr getByRoomId(RoomId roomId);

private:
    static std::map<int32_t, std::vector<Water13::Ptr>> roomsArr;
};


}

#endif
