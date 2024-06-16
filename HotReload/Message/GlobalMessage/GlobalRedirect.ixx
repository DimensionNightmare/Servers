module;
#include <coroutine>
#include <cstdint>
#include <list>
#include <string>

#include "StdMacro.h"
export module GlobalMessage:GlobalRedirect;

import DNTask;
import MessagePack;
import GlobalServerHelper;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;

namespace GlobalMessage
{

	export DNTaskVoid Msg_ReqAuthAccount(SocketChannelPtr channel, uint32_t msgId, Message* msg)
	{
		g2A_ResAuthAccount response;

		// if has db not need origin
		GlobalServerHelper* dnServer = GetGlobalServer();
		list<ServerEntity*> serverList = dnServer->GetServerEntityManager()->GetEntitysByType(ServerType::GateServer);

		list<ServerEntity*> tempList;
		for (ServerEntity* server : serverList)
		{
			if (server->HasFlag(ServerEntityFlag::Locked))
			{
				tempList.emplace_back(server);
			}
		}

		tempList.sort([](ServerEntity* lhs, ServerEntity* rhs) { return lhs->ConnNum() < rhs->ConnNum(); });


		string binData;
		if (tempList.empty())
		{
			response.set_state_code(4);
			DNPrint(0, LoggerLevel::Debug, "not exist GateServer");
		}
		else
		{

			ServerEntity* entity = tempList.front();
			DNPrint(0, LoggerLevel::Debug, "send to GateServer : %d", entity->ID());

			entity->ConnNum()++;

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
					response.set_state_code(5);

					entity->ConnNum()--;
				}
				else
				{
					response.set_server_ip(entity->ServerIp());
					response.set_server_port(entity->ServerPort());
				}

			}

			// DNPrint(0, LoggerLevel::Debug, "%s", response.DebugString().c_str());
		}

		response.SerializeToString(&binData);

		MessagePack(msgId, MsgDeal::Res, nullptr, binData);

		channel->write(binData);

		co_return;
	}
}