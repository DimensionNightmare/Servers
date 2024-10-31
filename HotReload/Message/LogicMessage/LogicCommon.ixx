module;
#include "StdMacro.h"
export module LogicMessage:LogicCommon;

import DNTask;
import FuncHelper;
import LogicServerHelper;
import Logger;
import Macro;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import DNClientProxyHelper;

namespace LogicMessage
{

	// client request
	export DNTaskVoid Evt_ReqRegistSrv()
	{
		LogicServerHelper* dnServer = GetLogicServer();
		DNClientProxyHelper* client = dnServer->GetCSock();
		
		DNPrint(0, EMLoggerLevel::Debug, "Client:%s, port:%hu", client->remote_host.c_str(), client->remote_port);
		
		client->EMRegistState() = EMRegistState::Registing;

		COM_ReqRegistSrv request;

		request.set_server_type((int)dnServer->GetServerType());

		if (uint32_t serverIndex = dnServer->ServerId())
		{
			request.set_server_id(serverIndex);
		}

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
		d2L_ReqRegistSrv request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}
		
		DNPrint(0, EMLoggerLevel::Debug, "ip Reqregist: %s, %d", channel->peeraddr().c_str(), request.server_type());

		COM_ResRegistSrv response;

		LogicServerHelper* dnServer = GetLogicServer();
		RoomEntityManagerHelper* entityMan = dnServer->GetRoomEntityManager();

		EMServerType regType = (EMServerType)request.server_type();
		const string& ipPort = channel->localaddr();

		if (regType != EMServerType::DedicatedServer || ipPort.empty())
		{
			response.set_success(false);
		}

		//exist?
		if (RoomEntity* entity = channel->getContext<RoomEntity>())
		{
			response.set_success(false);
		}

		else if (int serverId = request.server_id())
		{
			if (RoomEntity* entity = entityMan->GetEntity(serverId))
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
					entity->SetSock(channel);
					channel->setContext(entity);

					response.set_success(true);

					// Re-enroll
					entityMan->MountEntity(entity);
				}
			}
			else
			{
				response.set_success(true);
				response.set_server_id(serverId);
				response.set_server_type((uint8_t(dnServer->GetServerType())));

				entity = entityMan->AddEntity(serverId, request.map_id());
				entity->SetSock(channel);

				channel->setContext(entity);

				size_t pos = ipPort.find(":");
				entity->ServerIp() = ipPort.substr(0, pos);
				entity->ServerPort() = request.server_port();
			}
		}

		else if (RoomEntity* entity = entityMan->AddEntity(entityMan->GenRoomId(), request.map_id()))
		{
			size_t pos = ipPort.find(":");
			entity->ServerIp() = ipPort.substr(0, pos);
			entity->ServerPort() = request.server_port();

			DNPrint(0, EMLoggerLevel::Debug, "ds regist:%s:%d", entity->ServerIp().c_str(), entity->ServerPort());

			entity->SetSock(channel);

			channel->setContext(entity);

			response.set_success(true);
			response.set_server_id(entity->ID());
			response.set_server_type((uint8_t(dnServer->GetServerType())));
		}

		string binData;
		response.SerializeToString(&binData);

		MessagePackAndSend(msgId, EMMsgDeal::Res, nullptr, binData, channel);
	}

	export void Exe_RetChangeCtlSrv(SocketChannelPtr channel, string binMsg)
	{
		COM_RetChangeCtlSrv request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}
		LogicServerHelper* dnServer = GetLogicServer();
		DNClientProxyHelper* client = dnServer->GetCSock();

		TICK_MAINSPACE_SIGN_FUNCTION(DNClientProxy, RedirectClient, client, request.server_port(), request.server_ip());
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