#include "client_manager.h"
#include "gateway.h"

#include "protocol/protobuf/proto_manager.h"
#include "protocol/protobuf/public/client.codedef.h"

namespace gateway{

using namespace std::placeholders;

static ProcessId lobbyPid;
static ProcessId hallPid;

#define PUBLIC_MSG_TO_LOBBY(clientMsgName) \
ProtoManager::me().regHandler(PROTO_CODE_PUBLIC(clientMsgName), std::bind(&ClientManager::relayClientMsgToServer, this, lobbyPid, PROTO_CODE_PUBLIC(clientMsgName),  _1, _2));

#define PUBLIC_MSG_TO_HALL(clientMsgName) \
ProtoManager::me().regHandler(PROTO_CODE_PUBLIC(clientMsgName), std::bind(&ClientManager::relayClientMsgToServer, this, hallPid, PROTO_CODE_PUBLIC(clientMsgName),  _1, _2));

#define PUBLIC_MSG_TO_CLIENT(clientMsgName)\
ProtoManager::me().regHandler(PROTO_CODE_PUBLIC(clientMsgName), std::bind(&ClientManager::relayClientMsgToClient, this, PROTO_CODE_PUBLIC(clientMsgName), _1, _2));


void ClientManager::regClientMsgRelay()
{
    lobbyPid = ProcessId("lobby", 1);
    hallPid  = ProcessId("hall", 1);

    /************转发到lobby************/
//    PLATFORM_MSG_TO_LOBBY(

    PUBLIC_MSG_TO_LOBBY(C_SendChat)
    PUBLIC_MSG_TO_LOBBY(C_G13_CreateGame)
    PUBLIC_MSG_TO_LOBBY(C_G13_JoinGame)
    PUBLIC_MSG_TO_LOBBY(C_G13_GiveUp)
    PUBLIC_MSG_TO_LOBBY(C_G13_VoteFoAbortGame)
    PUBLIC_MSG_TO_LOBBY(C_G13_ReadyFlag)
    PUBLIC_MSG_TO_LOBBY(C_G13_BringOut)
    PUBLIC_MSG_TO_LOBBY(C_G13_ReqGameHistoryCount)
    PUBLIC_MSG_TO_LOBBY(C_G13_ReqGameHistoryDetial)
    PUBLIC_MSG_TO_LOBBY(C_G13_SimulationRound)
    PUBLIC_MSG_TO_LOBBY(C_G13_OnShareAdvByWeChat)
    PUBLIC_MSG_TO_LOBBY(C_G13_Payment)
    PUBLIC_MSG_TO_LOBBY(C_Exchange)

    PUBLIC_MSG_TO_LOBBY(C_G13_QuickStart)
    PUBLIC_MSG_TO_LOBBY(C_G13_ReplyHalfEnter)

    PUBLIC_MSG_TO_LOBBY(C_G13_VoteForQuickStart)
    PUBLIC_MSG_TO_LOBBY(C_G13_ChipInPoint)
    PUBLIC_MSG_TO_LOBBY(C_G13_SelectMultiple)

    PUBLIC_MSG_TO_LOBBY(C_G13Gold_GetOpenPlace)
    PUBLIC_MSG_TO_LOBBY(C_G13_EnterGoldPlace)
    PUBLIC_MSG_TO_LOBBY(C_G13Gold_ChangeRoom)
    PUBLIC_MSG_TO_LOBBY(C_G13Gold_QuickStart)

    /************转发到hall*************/
//    PUBLIC_MSG_TO_HALL()
    /*************转发到client**********/
    PUBLIC_MSG_TO_CLIENT(S_Chat)
    PUBLIC_MSG_TO_CLIENT(S_Notice)
    PUBLIC_MSG_TO_CLIENT(S_PlayerBasicData)
    PUBLIC_MSG_TO_CLIENT(S_G13_RoomAttr)
    PUBLIC_MSG_TO_CLIENT(S_G13_PlayersInRoom)
    PUBLIC_MSG_TO_CLIENT(S_G13_AbortGameOrNot)
    PUBLIC_MSG_TO_CLIENT(S_G13_VoteFailed)
    PUBLIC_MSG_TO_CLIENT(S_G13_PlayerQuited)
    PUBLIC_MSG_TO_CLIENT(S_G13_HandOfMine)
    PUBLIC_MSG_TO_CLIENT(S_G13_AllHands)
    PUBLIC_MSG_TO_CLIENT(S_G13_AllRounds)
    PUBLIC_MSG_TO_CLIENT(S_G13_GameHistoryCount)
    PUBLIC_MSG_TO_CLIENT(S_G13_GameHistoryDetial)
    PUBLIC_MSG_TO_CLIENT(S_G13_CalcRoundSimulationRet)
    PUBLIC_MSG_TO_CLIENT(S_G13_WechatSharingInfo)
    PUBLIC_MSG_TO_CLIENT(S_G13_QuickStartOrNot)
    PUBLIC_MSG_TO_CLIENT(S_G13_VoteStartFailed)
    PUBLIC_MSG_TO_CLIENT(S_G13_ChipInInfo)
    PUBLIC_MSG_TO_CLIENT(S_G13_BankerInfo)
    PUBLIC_MSG_TO_CLIENT(S_G13_MultipleInfo)
    PUBLIC_MSG_TO_CLIENT(S_G13_RenewNotice)
    PUBLIC_MSG_TO_CLIENT(S_G13Gold_OpenPlaceInfo)
}

}
