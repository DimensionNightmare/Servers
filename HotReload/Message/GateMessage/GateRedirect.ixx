module;
#include <coroutine>
#include <string>
#include <cstdint>
#include <list>

#include "StdMacro.h"
export module GateMessage:GateRedirect;

import MessagePack;
import GateServerHelper;
import Logger;
import DNTask;
import ThirdParty.Libhv;
import ThirdParty.PbGen;

namespace GateMessage
{

	export DNTaskVoid Exe_ReqLoadData(SocketChannelPtr channel, uint32_t msgId,  string binMsg)
	{
		L2D_ReqLoadData request;
		if(!request.ParseFromString(binMsg))
		{
			co_return;
		}
		D2L_ResLoadData response;

		GateServerHelper* dnServer = GetGateServer();
		ServerEntityManagerHelper* entityMan = dnServer->GetServerEntityManager();
		const list<ServerEntity*>& dbServers = entityMan->GetEntitysByType(ServerType::DatabaseServer);

		string binData;
		if (dbServers.empty())
		{
			response.set_state_code(1);
		}
		else
		{
			ServerEntity* entity = dbServers.front();

			DNServerProxyHelper* server = dnServer->GetSSock();
			uint32_t msgId = server->GetMsgId();

			// pack data
			binData = binMsg;
			MessagePack(msgId, MsgDeal::Req, request.GetDescriptor()->full_name().c_str(), binData);

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

	export DNTaskVoid Exe_ReqSaveData(SocketChannelPtr channel, uint32_t msgId,  string binMsg)
	{
		L2D_ReqSaveData request;
		if(!request.ParseFromString(binMsg))
		{
			co_return;
		}
		D2L_ResSaveData response;

		GateServerHelper* dnServer = GetGateServer();
		ServerEntityManagerHelper* entityMan = dnServer->GetServerEntityManager();
		const list<ServerEntity*>& dbServers = entityMan->GetEntitysByType(ServerType::DatabaseServer);

		string binData;
		if (dbServers.empty())
		{
			response.set_state_code(1);
		}
		else
		{
			ServerEntity* entity = dbServers.front();

			DNServerProxyHelper* server = dnServer->GetSSock();
			uint32_t msgId = server->GetMsgId();

			// pack data
			binData = binMsg;
			MessagePack(msgId, MsgDeal::Req, request.GetDescriptor()->full_name().c_str(), binData);

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
