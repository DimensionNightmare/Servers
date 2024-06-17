module;
#include <coroutine>
#include <cstdint>
#include <list>
#include <string>

#include "StdMacro.h"
export module GateMessage:GateClient;

import GateServerHelper;
import DNTask;
import StrUtils;
import MessagePack;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;

namespace GateMessage
{

	// client request
	export DNTaskVoid Msg_ReqAuthToken(SocketChannelPtr channel, uint32_t msgId,  string binMsg)
	{
		C2S_ReqAuthToken request;
		if(!request.ParseFromString(binMsg))
		{
			co_return;
		}

		GateServerHelper* dnServer = GetGateServer();
		ProxyEntityManagerHelper* entityMan = dnServer->GetProxyEntityManager();

		S2C_ResAuthToken response;
		string binData;

		ProxyEntity* entity = entityMan->GetEntity(request.account_id());
		if (!entity)
		{
			DNPrint(0, LoggerLevel::Debug, "noaccount %d!!", request.account_id());
			response.set_state_code(1);
		}
		// if not match, timer will destory entity
		else if (Md5Hash(entity->Token()) != request.token())
		{
			DNPrint(0, LoggerLevel::Debug, "not match!!");
			response.set_state_code(2);
		}
		else
		{
			DNPrint(0, LoggerLevel::Debug, "match!!");

			channel->setContext(entity);
			entity->SetSock(channel);

			if (uint64_t timerId = entity->TimerId())
			{
				entity->TimerId() = 0;
				entityMan->Timer()->killTimer(timerId);
			}

			//DS Server
			ServerEntityManagerHelper* serverEntityMan = dnServer->GetServerEntityManager();
			ServerEntity* serverEntity = nullptr;

			// <cache> server to load login data
			if (uint32_t serverId = entity->RecordServerId())
			{
				serverEntity = serverEntityMan->GetEntity(serverId);
			}

			// pool
			if (!serverEntity)
			{
				list<ServerEntity*> serverEntityList = serverEntityMan->GetEntitysByType(ServerType::LogicServer);
				if (serverEntityList.empty())
				{
					DNPrint(0, LoggerLevel::Debug, "Msg_ReqAuthToken not LogicServer !!");
					response.set_state_code(3);
				}
				else
				{
					serverEntity = serverEntityList.front();
				}
			}

			//req dedicatedServer Info to Login ds.
			if (serverEntity)
			{
				entity->RecordServerId() = serverEntity->ID();

				DNPrint(0, LoggerLevel::Debug, "Send to Logic index->%d, %d", entity->ID(), entity->RecordServerId());

				binData = binMsg;

				DNServerProxyHelper* server = dnServer->GetSSock();
				uint32_t msgId = server->GetMsgId();

				MessagePack(msgId, MsgDeal::Redir, request.GetDescriptor()->full_name().c_str(), binData);

				{
					auto taskGen = [](Message* msg) -> DNTask<Message*>
						{
							co_return msg;
						};
					auto dataChannel = taskGen(&response);
					server->AddMsg(msgId, &dataChannel, 9000);
					serverEntity->GetSock()->write(binData);
					co_await dataChannel;
					if (dataChannel.HasFlag(DNTaskFlag::Timeout))
					{
						response.set_state_code(4);
						DNPrint(0, LoggerLevel::Debug, "requst timeout! ");
					}

				}
			}

		}

		response.SerializeToString(&binData);

		MessagePack(msgId, MsgDeal::Res, nullptr, binData);

		channel->write(binData);

		co_return;
	}
}
