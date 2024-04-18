module;
#include <coroutine>
#include "hv/Channel.h"

#include "StdAfx.h"
#include "Server/S_Logic.pb.h"
export module LogicMessage:LogicGate;

import DNTask;
import MessagePack;
import LogicServerHelper;

using namespace std;
using namespace google::protobuf;
using namespace hv;
using namespace GMsg::S_Logic;

// client request
export void Exe_RetClientLogin(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	G2L_RetClientLogin* requset = reinterpret_cast<G2L_RetClientLogin*>(msg);

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
	ServerEntity* serverEntity = nullptr;
	if(!serverEntityList.empty())
	{
		serverEntity = serverEntityList.front();
	}

	if(serverEntity)
	{
		L2G_RetClientLogin retMsg;

		// retMsg.set_account_id(entity->);
		// retMsg.set_ip();
		// retMsg.set_port();
		// retMsg.set_token();
	}


}
