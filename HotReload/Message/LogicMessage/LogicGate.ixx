module;
#include <coroutine>
#include <cstdint>
#include <list>
#include "hv/Channel.h"

#include "StdMacro.h"
#include "Server/S_Gate_Logic.pb.h"
#include "Server/S_Logic_Dedicated.pb.h"
export module LogicMessage:LogicGate;

import DNTask;
import MessagePack;
import LogicServerHelper;
import Logger;

using namespace std;
using namespace google::protobuf;
using namespace hv;
using namespace GMsg;

namespace LogicMessage
{

	// client request
	export DNTaskVoid Msg_ReqClientLogin(SocketChannelPtr channel, uint32_t msgId, Message* msg)
	{
		G2L_ReqClientLogin* requset = reinterpret_cast<G2L_ReqClientLogin*>(msg);
		L2G_ResClientLogin response;

		LogicServerHelper* dnServer = GetLogicServer();
		ClientEntityManagerHelper* entityMan = dnServer->GetClientEntityManager();

		ClientEntity* entity = entityMan->AddEntity(requset->account_id());
		if (entity)
		{
			DNPrint(0, LoggerLevel::Debug, "AddEntity Client!");

		}
		else
		{
			DNPrint(0, LoggerLevel::Debug, "AddEntity Exist Client!");
			entity = entityMan->GetEntity(requset->account_id());
		}

		ServerEntityManagerHelper* serverEntityMan = dnServer->GetServerEntityManager();
		ServerEntity* serverEntity = nullptr;

		// cache
		if (uint32_t serverIdx = entity->ServerIndex())
		{
			serverEntity = serverEntityMan->GetEntity(serverIdx);
		}

		//pool
		if (!serverEntity)
		{
			list<ServerEntity*> serverEntityList = serverEntityMan->GetEntityByList(ServerType::DedicatedServer);
			if (serverEntityList.empty())
			{
				response.set_state_code(5);
				DNPrint(0, LoggerLevel::Debug, "not ds Server");
			}
			else
			{
				serverEntity = serverEntityList.front();
			}
		}

		string binData;

		// req token
		if (serverEntity)
		{

			DNServerProxyHelper* server = dnServer->GetSSock();
			uint32_t smsgId = server->GetMsgId();

			binData.clear();
			binData.resize(msg->ByteSizeLong());
			msg->SerializeToArray(binData.data(), binData.size());
			MessagePack(smsgId, MsgDeal::Req, L2D_ReqClientLogin::GetDescriptor()->full_name().c_str(), binData);

			{
				auto taskGen = [](Message* msg) -> DNTask<Message*>
					{
						co_return msg;
					};
				auto dataChannel = taskGen(&response);
				// wait data parse
				server->AddMsg(smsgId, &dataChannel, 8000);
				serverEntity->GetSock()->write(binData);
				co_await dataChannel;
				if (dataChannel.HasFlag(DNTaskFlag::Timeout))
				{
					DNPrint(0, LoggerLevel::Debug, "requst timeout! ");
					response.set_state_code(6);
				}
				else
				{
					entity->ServerIndex() = serverEntity->ID();
					//combin D2L_ReqClientLogin
					response.set_ip(serverEntity->ServerIp());
					response.set_port(serverEntity->ServerPort());
				}

			}

		}
		
		DNPrint(0, LoggerLevel::Debug, "ds:%s", response.DebugString().c_str());

		// pack data
		binData.clear();
		binData.resize(response.ByteSizeLong());
		response.SerializeToArray(binData.data(), binData.size());
		MessagePack(msgId, MsgDeal::Res, nullptr, binData);

		channel->write(binData);

		co_return;
	}

	export void Exe_RetAccountReplace(SocketChannelPtr channel, uint32_t msgId, Message* msg)
	{
		G2L_RetAccountReplace* requset = reinterpret_cast<G2L_RetAccountReplace*>(msg);

		LogicServerHelper* dnServer = GetLogicServer();
		ClientEntityManagerHelper* entityMan = dnServer->GetClientEntityManager();

		ClientEntity* entity = entityMan->GetEntity(requset->account_id());
		if (!entity)
		{
			DNPrint(0, LoggerLevel::Debug, "Client Entity Kick Not Exist !");
			return;
		}

		ServerEntityManagerHelper* serverEntityMan = dnServer->GetServerEntityManager();
		ServerEntity* serverEntity = serverEntityMan->GetEntity(entity->ServerIndex());

		// cache
		if (serverEntity)
		{
			string binData;
			binData.resize(msg->ByteSizeLong());
			msg->SerializeToArray(binData.data(), binData.size());

			MessagePack(0, MsgDeal::Ret, L2D_RetAccountReplace::GetDescriptor()->full_name().c_str(), binData);
			serverEntity->GetSock()->write(binData);
		}
		else
		{
			DNPrint(0, LoggerLevel::Debug, "Client Entity Kick Server Not Exist !");
		}

		// close entity save data
		entityMan->ClientEntityManagerHelper::RemoveEntity(entity->ID());
	}
}
