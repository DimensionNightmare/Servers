module;
#include <coroutine>
#include "hv/Channel.h"

#include "StdAfx.h"
#include "Server/S_Gate_Logic.pb.h"
#include "Server/S_Logic_Dedicated.pb.h"
export module LogicMessage:LogicGate;

import DNTask;
import MessagePack;
import LogicServerHelper;

using namespace std;
using namespace google::protobuf;
using namespace hv;
using namespace GMsg;

// client request
export DNTaskVoid Msg_ReqClientLogin(const SocketChannelPtr &channel, uint32_t msgId, Message *msg)
{
	G2L_ReqClientLogin* requset = reinterpret_cast<G2L_ReqClientLogin*>(msg);

	LogicServerHelper* dnServer = GetLogicServer();
	ClientEntityManagerHelper* entityMan = dnServer->GetClientEntityManager();

	ClientEntityHelper* entity = entityMan->AddEntity(requset->account_id());
	if (entity)
	{
		DNPrint(0, LoggerLevel::Debug, "AddEntity Client!");

	}
	else
	{
		DNPrint(0, LoggerLevel::Debug, "AddEntity Exist Client!");
		entity =  entityMan->GetEntity(requset->account_id());
	}


	ServerEntityManagerHelper* serverEntityMan = dnServer->GetServerEntityManager();
	ServerEntityHelper* serverEntity = nullptr;
	
	// cache
	if(uint32_t serverIdx = entity->ServerIndex())
	{
		serverEntity = serverEntityMan->GetEntity(serverIdx);
	}
	else
	{
		list<ServerEntity*> serverEntityList = serverEntityMan->GetEntityByList(ServerType::DedicatedServer);
		if(!serverEntityList.empty())
		{
			serverEntity = static_cast<ServerEntityHelper*>(serverEntityList.front());
		}
	}

	L2G_ResClientLogin response;

	string binData;

	// req token
	if(serverEntity)
	{
		
		auto dataChannel = [&response]()->DNTask<Message>
		{
			co_return response;
		}();

		DNServerProxyHelper* server = dnServer->GetSSock();
		uint32_t smsgId = server->GetMsgId();

		binData.clear();
		binData.resize(msg->ByteSize());
		msg->SerializeToArray(binData.data(), binData.size());
		MessagePack(smsgId, MsgDeal::Req, L2D_ReqClientLogin::GetDescriptor()->full_name().c_str(), binData);
		
		{

			// wait data parse
			server->AddMsg(smsgId, &dataChannel, 8000);
			serverEntity->GetSock()->write(binData);
			co_await dataChannel;
			if(dataChannel.HasFlag(DNTaskFlag::Timeout))
			{
				DNPrint(0, LoggerLevel::Debug, "requst timeout! \n");
				response.set_state_code(2);
			}
			else
			{
				//combin D2L_ReqClientLogin
				response.set_ip(serverEntity->ServerIp());
				response.set_port(serverEntity->ServerPort());
			}

		}

		DNPrint(0, LoggerLevel::Debug, "ds:%s", response.DebugString().c_str());
	}
	else
	{
		response.set_state_code(1);
		DNPrint(0, LoggerLevel::Debug, "not ds connect");
	}

	// pack data
	binData.clear();
	binData.resize(response.ByteSizeLong());
	response.SerializeToArray(binData.data(), binData.size());
	MessagePack(msgId, MsgDeal::Res, nullptr, binData);

	channel->write(binData);

	co_return;
}

export void Exe_RetAccountReplace(const SocketChannelPtr &channel, uint32_t msgId, Message *msg)
{
	G2L_RetAccountReplace* requset = reinterpret_cast<G2L_RetAccountReplace*>(msg);

	LogicServerHelper* dnServer = GetLogicServer();
	ClientEntityManagerHelper* entityMan = dnServer->GetClientEntityManager();

	ClientEntityHelper* entity = entityMan->GetEntity(requset->account_id());
	if (!entity)
	{
		DNPrint(0, LoggerLevel::Debug, "Client Entity Kick Not Exist !");
		return;
	}

	ServerEntityManagerHelper* serverEntityMan = dnServer->GetServerEntityManager();
	ServerEntityHelper* serverEntity = nullptr;
	
	// cache
	if(serverEntity = serverEntityMan->GetEntity(entity->ServerIndex()))
	{
		string binData;
		binData.resize(msg->ByteSize());
		msg->SerializeToArray(binData.data(), binData.size());

		MessagePack(0, MsgDeal::Ret, L2D_RetAccountReplace::GetDescriptor()->full_name().c_str(), binData);
		serverEntity->GetSock()->write(binData);
	}
	else
	{
		DNPrint(0, LoggerLevel::Debug, "Client Entity Kick Server Not Exist !");
	}

	// close entity save data
	entityMan->RemoveEntity(entity->ID());
}
