module;
#include <coroutine>
#include "hv/Channel.h"

#include "StdAfx.h"
#include "Server/S_Logic.pb.h"
#include "Client/C_Auth.pb.h"
export module LogicMessage:LogicGate;

import DNTask;
import MessagePack;
import LogicServerHelper;

using namespace std;
using namespace google::protobuf;
using namespace hv;
using namespace GMsg::S_Logic;
using namespace GMsg::C_Auth;

// client request
export void Msg_ReqClientLogin(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	G2L_ReqClientLogin* requset = reinterpret_cast<G2L_ReqClientLogin*>(msg);

	auto entityMan = GetLogicServer()->GetClientEntityManager();

	ClientEntityHelper* entity = nullptr;
	if (entity = entityMan->AddEntity(requset->account_id()))
	{
		DNPrint(-1, LoggerLevel::Debug, "AddEntity Client!");

	}
	else
	{
		DNPrint(-1, LoggerLevel::Debug, "AddEntity Exist Client!");
		entity =  entityMan->GetEntity(requset->account_id());
	}


	auto serverEntityMan = GetLogicServer()->GetServerEntityManager();
	list<ServerEntity*> serverEntityList = serverEntityMan->GetEntityByList(ServerType::DedicatedServer);
	ServerEntityHelper* serverEntity = nullptr;
	if(!serverEntityList.empty())
	{
		ServerEntityHelper* serverEntity = static_cast<ServerEntityHelper*>(serverEntityList.front());
	}

	L2G_ResClientLogin response;

	if(serverEntity)
	{
		S2C_RetClientLogin* cliMsg = response.mutable_ds_info();
		cliMsg->set_ip(serverEntity->ServerIp());
		cliMsg->set_port(serverEntity->ServerPort());
	}

	// pack data
	string binData;
	binData.resize(response.ByteSizeLong());
	response.SerializeToArray(binData.data(), (int)binData.size());
	MessagePack(msgId, MsgDeal::Res, nullptr, binData);

	channel->write(binData);
}
