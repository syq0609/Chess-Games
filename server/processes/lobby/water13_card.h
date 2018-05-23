#ifndef LOBBY_WATER13_CARD_HPP
#define LOBBY_WATER13_CARD_HPP

#include "room.h"

#include "protocol/protobuf/proto_manager.h"

#include "deck.h"
#include "water13.h"

namespace lobby{

class Client;

class Water13_Card : public Water13
{
    friend class Water13;
    using ClientPtr = std::shared_ptr<Client>;

private:
    CREATE_FUN_NEW(Water13_Card);
    TYPEDEF_PTR(Water13_Card);
    using Water13::Water13;

    void trySettleAll();
    void settleAll();

    void checkAllVotes();
    void checkAllVotesQuickStart();
    void checkAllRenewReply();

    virtual void giveUp(ClientPtr client);
    virtual void readyFlag(const ProtoMsgPtr& proto, ClientPtr client);
    virtual void bringOut(const ProtoMsgPtr& proto, ClientPtr client);

private://消息处理      //房卡特有协议
    static void proto_C_G13_CreateGame(ProtoMsgPtr proto, ClientConnectionId ccid);
    static void proto_C_G13_JoinGame(ProtoMsgPtr proto, ClientConnectionId ccid);
    static void proto_C_G13_VoteFoAbortGame(ProtoMsgPtr proto, ClientConnectionId ccid);
    static void proto_C_G13_QuickStart(ProtoMsgPtr proto, ClientConnectionId ccid);
    static void proto_C_G13_VoteForQuickStart(ProtoMsgPtr proto, ClientConnectionId ccid);
    static void proto_C_G13_ReplyHalfEnter(ProtoMsgPtr proto, ClientConnectionId ccid);
    static void proto_C_G13_RenewRoom(ProtoMsgPtr proto, ClientConnectionId ccid);
    static void proto_C_G13_RenewReply(ProtoMsgPtr proto, ClientConnectionId ccid);

public:
    static void regMsgHandler();
    static Water13_Card::Ptr getPtr(Water13::Ptr& ptr);
    static Ptr getByRoomId(RoomId roomId);
};

}

#endif
