module;
export module GateMessage:GateClient;

import GateServerHelper;
import DNTask;
import StrUtils;
import FuncHelper;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import ProxyEntityManagerHelper;
import std.compat;

namespace GateMessage
{

	// client request
	export DNTaskVoid Msg_ReqAuthToken(hv::SocketChannelPtr channel, uint32_t msgId, std::string binMsg)
	{
		GMsg::C2S_ReqAuthToken request;
		if(!request.ParseFromString(binMsg))
		{
			co_return;
		}

		GateServerHelper* dnServer = GetGateServer();
		ProxyEntityManagerHelper* entityMan = dnServer->GetProxyEntityManager();

		GMsg::S2C_ResAuthToken response;
		std::string binData;

		ProxyEntity* entity = entityMan->GetEntity(request.account_id());
		if (!entity)
		{
			LoggerPrint()(ELogLevel_Debug, "noaccount {}!!", request.account_id());
			response.set_state_code(1);
		}
		// if not match, timer will destory entity
		else if (Md5Hash(entity->Token()) != request.token())
		{
			LoggerPrint()(ELogLevel_Debug, "not match!!");
			response.set_state_code(2);
		}
		else
		{
			LoggerPrint()(ELogLevel_Debug, "match!!");

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
				std::list<ServerEntity*> serverEntityList = serverEntityMan->GetEntitysByType(EMServerType::LogicServer);
				if (serverEntityList.empty())
				{
					LoggerPrint()(ELogLevel_Debug, "Msg_ReqAuthToken not LogicServer !!");
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

				auto taskGen = [](Message* msg) -> DNTask<Message*>
					{
						co_return msg;
					};
				auto dataChannel = taskGen(&response);

				DNServerProxyHelper* server = dnServer->GetSSock();
				uint32_t msgId = server->GetMsgId();
				server->AddMsg(msgId, &dataChannel, 9000);

				binData = binMsg;

				MessagePackAndSend(msgId, EMMsgDeal::Redir, request.GetDescriptor()->full_name(), binData, serverEntity->GetSock());
				
				co_await dataChannel;
				if (dataChannel.HasFlag(EMDNTaskFlag::Timeout))
				{
					response.set_state_code(4);
					LoggerPrint()(ELogLevel_Debug, "requst timeout! ");
				}

			}

		}

		response.SerializeToString(&binData);

		MessagePackAndSend(msgId, EMMsgDeal::Res, "", binData, channel);

		co_return;
	}
}
