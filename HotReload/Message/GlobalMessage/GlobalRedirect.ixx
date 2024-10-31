module;
#include "StdMacro.h"
export module GlobalMessage:GlobalRedirect;

import DNTask;
import FuncHelper;
import GlobalServerHelper;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import ServerEntity;
import ServerEntityManagerHelper;

namespace GlobalMessage
{

	export DNTaskVoid Msg_ReqAuthAccount(SocketChannelPtr channel, uint32_t msgId,  string binMsg)
	{
		A2g_ReqAuthAccount request;
		if(!request.ParseFromString(binMsg))
		{
			co_return;
		}
		g2A_ResAuthAccount response;

		// if has db not need origin
		GlobalServerHelper* dnServer = GetGlobalServer();
		list<ServerEntity*> serverList = dnServer->GetServerEntityManager()->GetEntitysByType(EMServerType::GateServer);

		list<ServerEntity*> tempList;
		for (ServerEntity* server : serverList)
		{
			if (server->HasFlag(EMServerEntityFlag::Locked))
			{
				tempList.emplace_back(server);
			}
		}

		tempList.sort([](ServerEntity* lhs, ServerEntity* rhs) { return lhs->ConnNum() < rhs->ConnNum(); });


		string binData;
		if (tempList.empty())
		{
			response.set_state_code(4);
			DNPrint(0, EMLoggerLevel::Debug, "not exist GateServer");
		}
		else
		{
			ServerEntity* entity = tempList.front();
			DNPrint(0, EMLoggerLevel::Debug, "send to GateServer : %d", entity->ID());

			entity->ConnNum()++;

			// pack data
			binData = binMsg;

			// data alloc
			auto taskGen = [](Message* msg) -> DNTask<Message*>
				{
					co_return msg;
				};
			auto dataChannel = taskGen(&response);

			DNServerProxyHelper* server = dnServer->GetSSock();
			uint32_t msgId = server->GetMsgId();

			server->AddMsg(msgId, &dataChannel, 8000);
			
			MessagePackAndSend(msgId, EMMsgDeal::Req, request.GetDescriptor()->full_name().c_str(), binData, entity->GetSock());

			co_await dataChannel;
			if (dataChannel.HasFlag(EMDNTaskFlag::Timeout))
			{
				DNPrint(0, EMLoggerLevel::Debug, "requst timeout! ");
				response.set_state_code(5);

				entity->ConnNum()--;
			}
			else
			{
				response.set_server_ip(entity->ServerIp());
				response.set_server_port(entity->ServerPort());
			}

			

			// DNPrint(0, EMLoggerLevel::Debug, "%s", response.DebugString().c_str());
		}

		response.SerializeToString(&binData);

		MessagePackAndSend(msgId, EMMsgDeal::Res, nullptr, binData, channel);

		co_return;
	}
}