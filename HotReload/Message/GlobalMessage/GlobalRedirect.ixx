module;
export module GlobalMessage:GlobalRedirect;

import DNTask;
import FuncHelper;
import GlobalServerHelper;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import ServerEntity;
import ServerEntityManagerHelper;
import std.compat;

namespace GlobalMessage
{

	export DNTaskVoid Msg_ReqAuthAccount(hv::SocketChannelPtr channel, uint32_t msgId, std::string binMsg)
	{
		GMsg::A2g_ReqAuthAccount request;
		if(!request.ParseFromString(binMsg))
		{
			co_return;
		}
		GMsg::g2A_ResAuthAccount response;

		// if has db not need origin
		GlobalServerHelper* dnServer = GetGlobalServer();
		std::list<ServerEntity::Ptr> serverList = dnServer->GetServerEntityManager()->GetEntitysByType(EMServerType::GateServer);

		std::list<ServerEntity::Ptr> tempList;
		for (ServerEntity::Ptr server : serverList)
		{
			if (server->HasFlag(EMServerEntityFlag::Locked))
			{
				tempList.emplace_back(server);
			}
		}

		tempList.sort([](ServerEntity::Ptr lhs, ServerEntity::Ptr rhs) { return lhs->ConnNum() < rhs->ConnNum(); });


		std::string binData;
		if (tempList.empty())
		{
			response.set_state_code(4);
			SPidLogger.Record(ELogLevel_Debug, "not exist GateServer");
		}
		else
		{
			ServerEntity::Ptr entity = tempList.front();
			SPidLogger.Record(ELogLevel_Debug, "send to GateServer : {}", entity->ID());

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
			
			MessagePackAndSend(msgId, EMMsgDeal::Req, request.GetDescriptor()->full_name(), binData, entity->GetSock());

			co_await dataChannel;
			if (dataChannel.HasFlag(EMDNTaskFlag::Timeout))
			{
				SPidLogger.Record(ELogLevel_Debug, "requst timeout! ");
				response.set_state_code(5);

				entity->ConnNum()--;
			}
			else
			{
				response.set_server_ip(entity->ServerIp());
				response.set_server_port(entity->ServerPort());
			}

			

			SPidLogger.Record(ELogLevel_Debug, response.DebugString());
		}

		response.SerializeToString(&binData);

		MessagePackAndSend(msgId, EMMsgDeal::Res, "", binData, channel);

		co_return;
	}
}