module;
#include <coroutine>
#include <cstdint>
#include <list>
#include <string>

#include "StdMacro.h"
export module ControlMessage:ControlRedirect;

import DNTask;
import FuncHelper;
import ControlServerHelper;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;

namespace ControlMessage
{
	export DNTaskVoid Msg_ReqAuthAccount(SocketChannelPtr channel, uint32_t msgId,  string binMsg)
	{
		A2g_ReqAuthAccount request;
		if(!request.ParseFromString(binMsg))
		{
			co_return;
		}
		g2A_ResAuthAccount response;

		ServerEntity* entity = nullptr;
		const list<ServerEntity*>& serverList = GetControlServer()->GetServerEntityManager()->GetEntitysByType(ServerType::GlobalServer);

		// std::erase_if(serverList, [](ServerEntity* itor){return itor ? itor->TimerId() : true; });
		// serverList.sort([](ServerEntity* lhs, ServerEntity* rhs){return lhs->ConnNum() < rhs->ConnNum(); });

		for (ServerEntity* server : serverList)
		{
			if (server->TimerId())
			{
				continue;
			}

			if (!entity)
			{
				entity = server;
				continue;
			}

			if (server->ConnNum() < entity->ConnNum())
			{
				entity = server;
			}
		}

		string binData;

		if (!entity)
		{
			response.set_state_code(2);
		}
		else
		{

			// message change to global
			auto taskGen = [](Message* msg) -> DNTask<Message*>
				{
					co_return msg;
				};
			auto dataChannel = taskGen(&response);
			// wait data parse

			DNServerProxyHelper* server = GetControlServer()->GetSSock();

			uint32_t msgId = server->GetMsgId();
			server->AddMsg(msgId, &dataChannel, 9000);

			binData = binMsg;
			MessagePackAndSend(msgId, MsgDeal::Redir, request.GetDescriptor()->full_name().c_str(), binData, entity->GetSock());

			co_await dataChannel;
			if (dataChannel.HasFlag(DNTaskFlag::Timeout))
			{
				DNPrint(0, LoggerLevel::Debug, "requst timeout! ");
				response.set_state_code(3);
			}

		}

		response.SerializeToString(&binData);

		MessagePackAndSend(msgId, MsgDeal::Res, nullptr, binData, channel);
		co_return;
	}
}
