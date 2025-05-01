module;
export module GateMessage:GateRedirect;

import FuncHelper;
import GateServerHelper;
import Logger;
import DNTask;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import ServerEntityManagerHelper;
import std.compat;

namespace GateMessage
{

	export DNTaskVoid Exe_ReqLoadData(hv::SocketChannelPtr channel, uint32_t msgId, std::string binMsg)
	{
		GMsg::L2D_ReqLoadData request;
		if(!request.ParseFromString(binMsg))
		{
			co_return;
		}
		GMsg::D2L_ResLoadData response;

		GateServerHelper* dnServer = GetGateServer();
		ServerEntityManagerHelper* entityMan = dnServer->GetServerEntityManager();
		const std::list<ServerEntity*>& dbServers = entityMan->GetEntitysByType(EMServerType::DatabaseServer);

		std::string binData;
		if (dbServers.empty())
		{
			response.set_state_code(1);
		}
		else
		{
			ServerEntity* entity = dbServers.front();

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
				LoggerPrint()(ELogLevel_Debug, "requst timeout! ");
				response.set_state_code(2);
			}
			

		}

		response.SerializeToString(&binData);
		MessagePackAndSend(msgId, EMMsgDeal::Res, "", binData, channel);

		co_return;
	}

	export DNTaskVoid Exe_ReqSaveData(hv::SocketChannelPtr channel, uint32_t msgId, std::string binMsg)
	{
		GMsg::L2D_ReqSaveData request;
		if(!request.ParseFromString(binMsg))
		{
			co_return;
		}
		GMsg::D2L_ResSaveData response;

		GateServerHelper* dnServer = GetGateServer();
		ServerEntityManagerHelper* entityMan = dnServer->GetServerEntityManager();
		const std::list<ServerEntity*>& dbServers = entityMan->GetEntitysByType(EMServerType::DatabaseServer);

		std::string binData;
		if (dbServers.empty())
		{
			response.set_state_code(1);
		}
		else
		{
			ServerEntity* entity = dbServers.front();

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
				LoggerPrint()(ELogLevel_Debug, "requst timeout! ");
				response.set_state_code(2);
			}
			
		}

		response.SerializeToString(&binData);
		MessagePackAndSend(msgId, EMMsgDeal::Res, "", binData, channel);

		co_return;
	}
}
