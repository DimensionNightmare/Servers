module;
export module LogicMessage:LogicCommon;

import DNTask;
import FuncHelper;
import LogicServerHelper;
import Logger;
import DllUtils;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import DNClientProxyHelper;

#define FUNCPLACE(func) #func, func

namespace LogicMessage
{

	// client request
	export DNTaskVoid Evt_ReqRegistSrv()
	{
		LogicServerHelper* dnServer = GetLogicServer();
		DNClientProxyHelper* client = dnServer->GetCSock();
		
		LoggerPrint()(ELogLevel_Debug, "Client:{}, port:{}", client->remote_host, client->remote_port);
		
		client->EMRegistState() = EMRegistState::Registing;

		COM_ReqRegistSrv request;

		request.set_server_type((int)dnServer->GetServerType());

		if (uint32_t serverIndex = dnServer->ServerId())
		{
			request.set_server_id(serverIndex);
		}

		// pack data
		std::string binData;
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
			MessagePackAndSend(msgId, EMMsgDeal::Req, request.GetDescriptor()->full_name(), binData, client->GetChannel());
			co_await dataChannel;
			if (dataChannel.HasFlag(EMDNTaskFlag::Timeout))
			{
				LoggerPrint()(ELogLevel_Debug, "requst timeout! ");
			}

		}

		if (response.success())
		{
			LoggerPrint()(ELogLevel_Debug, "regist Server success! Rec index:{}", response.server_id());
			client->EMRegistState() = EMRegistState::Registed;
			client->RegistType() = response.server_type();
			dnServer->ServerId() = response.server_id();
		}
		else
		{
			LoggerPrint()(ELogLevel_Debug, "regist Server error!  ");
			// dnServer->IsRun() = false; //exit application
			client->EMRegistState() = EMRegistState::None;
		}

		co_return;
	}

	// client request
	export void Msg_ReqRegistSrv(SocketChannelPtr channel, uint32_t msgId, std::string binMsg)
	{
		d2L_ReqRegistSrv request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}
		
		LoggerPrint()(ELogLevel_Debug, "ip Reqregist: {}, {}", channel->peeraddr(), request.server_type());

		COM_ResRegistSrv response;

		LogicServerHelper* dnServer = GetLogicServer();
		RoomEntityManagerHelper* entityMan = dnServer->GetRoomEntityManager();

		EMServerType regType = (EMServerType)request.server_type();
		const std::string& ipPort = channel->localaddr();

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

			LoggerPrint()(ELogLevel_Debug, "ds regist:{}:{}", entity->ServerIp(), entity->ServerPort());

			entity->SetSock(channel);

			channel->setContext(entity);

			response.set_success(true);
			response.set_server_id(entity->ID());
			response.set_server_type((uint8_t(dnServer->GetServerType())));
		}

		std::string binData;
		response.SerializeToString(&binData);

		MessagePackAndSend(msgId, EMMsgDeal::Res, "", binData, channel);
	}

	export void Exe_RetChangeCtlSrv(SocketChannelPtr channel, std::string binMsg)
	{
		COM_RetChangeCtlSrv request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}
		LogicServerHelper* dnServer = GetLogicServer();
		DNClientProxyHelper* client = dnServer->GetCSock();

		TickMainSpaceDll(client, FUNCPLACE(&DNClientProxy::RedirectClient),  request.server_port(), request.server_ip());
	}

	export void Exe_RetHeartbeat(SocketChannelPtr channel, std::string binMsg)
	{
		COM_RetHeartbeat request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}
	}
}