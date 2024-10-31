module;
#include "StdMacro.h"
export module GlobalMessage:GlobalCommon;

import DNTask;
import FuncHelper;
import GlobalServerHelper;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import DNClientProxyHelper;
import DNServerProxyHelper;

namespace GlobalMessage
{

	// client request
	export DNTaskVoid Evt_ReqRegistSrv()
	{
		GlobalServerHelper* dnServer = GetGlobalServer();
		DNClientProxyHelper* client = dnServer->GetCSock();
		DNServerProxyHelper* server = dnServer->GetSSock();
		
		DNPrint(0, EMLoggerLevel::Debug, "Client:%s, port:%hu", client->remote_host.c_str(), client->remote_port);
		
		client->EMRegistState() = EMRegistState::Registing;

		COM_ReqRegistSrv request;

		request.set_server_type((int)dnServer->GetServerType());

		if (uint32_t serverId = dnServer->ServerId())
		{
			request.set_server_id(serverId);
		}

		request.set_server_port(server->port);

		// pack data
		string binData;
		request.SerializeToString(&binData);
		
		// data alloc
		COM_ResRegistSrv response;

		{
			auto taskGen = [](Message* msg) -> DNTask<Message*>
				{
					co_return msg;
				};
			auto dataChannel = taskGen(&response);
			
			uint32_t msgId = client->GetMsgId();
			client->AddMsg(msgId, &dataChannel);
			MessagePackAndSend(msgId, EMMsgDeal::Req, request.GetDescriptor()->full_name().c_str(), binData, client->GetChannel());
			
			co_await dataChannel;
			if (dataChannel.HasFlag(EMDNTaskFlag::Timeout))
			{
				DNPrint(0, EMLoggerLevel::Debug, "requst timeout! ");
			}

		}

		if (response.success())
		{
			DNPrint(0, EMLoggerLevel::Debug, "regist Server success! Rec index:%d", response.server_id());
			client->EMRegistState() = EMRegistState::Registed;
			client->RegistType() = response.server_type();
			dnServer->ServerId() = response.server_id();
		}
		else
		{
			DNPrint(0, EMLoggerLevel::Debug, "regist Server error!  ");
			// dnServer->IsRun() = false; //exit application
			client->EMRegistState() = EMRegistState::None;
		}


		co_return;
	}

	// client request
	export void Msg_ReqRegistSrv(SocketChannelPtr channel, uint32_t msgId,  string binMsg)
	{
		COM_ReqRegistSrv request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}
		
		DNPrint(0, EMLoggerLevel::Debug, "ip Reqregist: %s, %d", channel->peeraddr().c_str(), request.server_type());

		COM_ResRegistSrv response;

		GlobalServerHelper* dnServer = GetGlobalServer();
		ServerEntityManagerHelper* entityMan = dnServer->GetServerEntityManager();

		EMServerType regType = (EMServerType)request.server_type();

		const string& ipPort = channel->localaddr();

		if (regType < EMServerType::GateServer || regType > EMServerType::LogicServer || ipPort.empty())
		{
			response.set_success(false);
		}

		//exist?
		else if (ServerEntity* entity = channel->getContext<ServerEntity>())
		{
			response.set_success(false);
		}

		// take task to regist !
		else if (uint32_t serverId = request.server_id())
		{
			if (ServerEntity* entity = entityMan->GetEntity(serverId))
			{
				// wait destroy`s destroy
				if (uint64_t timerId = entity->TimerId())
				{
					entity->TimerId() = 0;
					entityMan->Timer()->killTimer(timerId);
				}

				// already connect
				if (const SocketChannelPtr& sock = entity->GetSock())
				{
					response.set_success(false);
				}
				else
				{
					entity->LinkNode() = nullptr;
					entity->SetSock(channel);
					channel->setContext(entity);

					response.set_success(true);

					// Re-enroll
					entityMan->MountEntity(regType, entity);
				}
			}
			else
			{
				response.set_success(true);
				response.set_server_id(serverId);
				response.set_server_type((uint8_t(dnServer->GetServerType())));

				entity = entityMan->AddEntity(serverId, regType);
				entity->SetSock(channel);

				channel->setContext(entity);

				size_t pos = ipPort.find(":");
				entity->ServerIp() = ipPort.substr(0, pos);
				entity->ServerPort() = request.server_port();
			}

		}

		else if (ServerEntity* entity = entityMan->AddEntity(entityMan->GenServerId(), regType))
		{
			size_t pos = ipPort.find(":");
			entity->ServerIp() = ipPort.substr(0, pos);
			entity->ServerPort() = request.server_port();
			entity->SetSock(channel);

			channel->setContext(entity);

			response.set_success(true);
			response.set_server_id(entity->ID());
			response.set_server_type((uint8_t(dnServer->GetServerType())));
		}

		string binData;
		response.SerializeToString(&binData);

		MessagePackAndSend(msgId, EMMsgDeal::Res, nullptr, binData, channel);

		if (response.success())
		{
			dnServer->UpdateServerGroup();
		}

	}

	export void Exe_RetHeartbeat(SocketChannelPtr channel, string binMsg)
	{
		COM_RetHeartbeat request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}
	}
}
