module;
#include <coroutine>
#include <cstdint>
#include <list>
#include "hv/Channel.h"

#include "StdMacro.h"
#include "Client/C_Auth.pb.h"
export module LogicMessage:LogicRedirect;

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
	export void Exe_RetAccountReplace(SocketChannelPtr channel, uint32_t msgId, Message* msg)
	{
		S2C_RetAccountReplace* request = reinterpret_cast<S2C_RetAccountReplace*>(msg);

		LogicServerHelper* dnServer = GetLogicServer();
		ClientEntityManagerHelper* entityMan = dnServer->GetClientEntityManager();

		ClientEntity* entity = entityMan->GetEntity(request->account_id());
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
			msg->SerializeToString(&binData);

			MessagePack(0, MsgDeal::Ret, msg->GetDescriptor()->full_name().c_str(), binData);
			serverEntity->GetSock()->write(binData);
		}
		else
		{
			DNPrint(0, LoggerLevel::Debug, "Client Entity Kick Server Not Exist !");
		}

		// close entity save data
		entityMan->RemoveEntity(entity->ID());
	}
	
	// client request
	export DNTaskVoid Msg_ReqClientLogin(SocketChannelPtr channel, uint32_t msgId, Message* msg)
	{
		C2S_ReqAuthToken* request = reinterpret_cast<C2S_ReqAuthToken*>(msg);
		S2C_ResAuthToken response;

		LogicServerHelper* dnServer = GetLogicServer();
		ClientEntityManagerHelper* entityMan = dnServer->GetClientEntityManager();

		ClientEntity* entity = entityMan->AddEntity(request->account_id());
		if (entity)
		{
			DNPrint(0, LoggerLevel::Debug, "AddEntity Client!");

		}
		else
		{
			DNPrint(0, LoggerLevel::Debug, "AddEntity Exist Client!");
			entity = entityMan->GetEntity(request->account_id());
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
			uint32_t msgId = server->GetMsgId();

			msg->SerializeToString(&binData);
			MessagePack(msgId, MsgDeal::Req, request->GetDescriptor()->full_name().c_str(), binData);

			{
				auto taskGen = [](Message* msg) -> DNTask<Message*>
					{
						co_return msg;
					};
				auto dataChannel = taskGen(&response);
				// wait data parse
				server->AddMsg(msgId, &dataChannel, 8000);
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
					//combin
					response.set_ip(serverEntity->ServerIp());
					response.set_port(serverEntity->ServerPort());
				}

			}

		}

		DNPrint(0, LoggerLevel::Debug, "ds:%s", response.DebugString().c_str());

		// pack data
		response.SerializeToString(&binData);
		MessagePack(msgId, MsgDeal::Res, nullptr, binData);

		channel->write(binData);

		co_return;
	}

}
