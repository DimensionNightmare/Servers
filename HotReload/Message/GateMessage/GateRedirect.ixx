module;
#include <coroutine>
#include <string>
#include <cstdint>
#include <list>
#include "hv/Channel.h"

#include "StdMacro.h"
#include "Server/S_Logic.pb.h"
export module GateMessage:GateRedirect;

import MessagePack;
import GateServerHelper;
import Logger;
import DNTask;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg;

namespace GateMessage
{

	export DNTaskVoid Exe_ReqLoadData(SocketChannelPtr channel, uint32_t msgId, Message* msg)
	{
		L2D_ReqLoadData* request = reinterpret_cast<L2D_ReqLoadData*>(msg);
		D2L_ResLoadData response;

		GateServerHelper* dnServer = GetGateServer();
		ServerEntityManagerHelper* entityMan = dnServer->GetServerEntityManager();
		const list<ServerEntity*>& dbServers = entityMan->GetEntityByList(ServerType::DatabaseServer);

		string binData;
		if(dbServers.empty())
		{
			response.set_state_code(1);
		}
		else
		{
			ServerEntity* entity = dbServers.front();

			DNServerProxyHelper* server = dnServer->GetSSock();
			uint32_t msgId = server->GetMsgId();

			// pack data
			msg->SerializeToString(&binData);
			MessagePack(msgId, MsgDeal::Req, msg->GetDescriptor()->full_name().c_str(), binData);

			{
				// data alloc
				auto taskGen = [](Message* msg) -> DNTask<Message*>
					{
						co_return msg;
					};
				auto dataChannel = taskGen(&response);
				
				server->AddMsg(msgId, &dataChannel, 8000);
				entity->GetSock()->write(binData);
				co_await dataChannel;
				if (dataChannel.HasFlag(DNTaskFlag::Timeout))
				{
					DNPrint(0, LoggerLevel::Debug, "requst timeout! ");
					response.set_state_code(2);
				}
			}


		}

		response.SerializeToString(&binData);
		MessagePack(msgId, MsgDeal::Res, nullptr, binData);

		channel->write(binData);

		co_return;
	}

	export DNTaskVoid Exe_ReqSaveData(SocketChannelPtr channel, uint32_t msgId, Message* msg)
	{
		L2D_ReqSaveData* request = reinterpret_cast<L2D_ReqSaveData*>(msg);
		D2L_ResSaveData response;

		GateServerHelper* dnServer = GetGateServer();
		ServerEntityManagerHelper* entityMan = dnServer->GetServerEntityManager();
		const list<ServerEntity*>& dbServers = entityMan->GetEntityByList(ServerType::DatabaseServer);

		string binData;
		if(dbServers.empty())
		{
			response.set_state_code(1);
		}
		else
		{
			ServerEntity* entity = dbServers.front();

			DNServerProxyHelper* server = dnServer->GetSSock();
			uint32_t msgId = server->GetMsgId();

			// pack data
			msg->SerializeToString(&binData);
			MessagePack(msgId, MsgDeal::Req, msg->GetDescriptor()->full_name().c_str(), binData);

			{
				// data alloc
				auto taskGen = [](Message* msg) -> DNTask<Message*>
					{
						co_return msg;
					};
				auto dataChannel = taskGen(&response);
				
				server->AddMsg(msgId, &dataChannel, 8000);
				entity->GetSock()->write(binData);
				co_await dataChannel;
				if (dataChannel.HasFlag(DNTaskFlag::Timeout))
				{
					DNPrint(0, LoggerLevel::Debug, "requst timeout! ");
					response.set_state_code(2);
				}
			}


		}

		response.SerializeToString(&binData);
		MessagePack(msgId, MsgDeal::Res, nullptr, binData);

		channel->write(binData);

		co_return;
	}
}
