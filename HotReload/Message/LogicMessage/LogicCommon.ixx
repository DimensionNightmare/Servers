module;
export module LogicMessage:LogicCommon;

import DNTask;
import FuncHelper;
import Logger;
import DllUtils;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import DNClientProxyHelper;
import DNServer;
import RoomEntity;
import LogicServerHelper;
import ECSW;

#define FUNCPLACE(class, func) &class::func, #class"_"#func

namespace LogicMessage
{

	// client request
	export DNTaskVoid Evt_ReqRegistSrv(const DNServer::Ptr& server)
	{
		LogicServerHelper::Ptr dnServer = server->GetSelf<LogicServerHelper>();

		DNClientProxyHelper::Ptr clientProxy = dnServer->GetClientProxy();
		
		dnServer->GetLogger()->Record(ELogLevel_Debug, "Client:{}, port:{}", clientProxy->remote_host, clientProxy->remote_port);
		
		clientProxy->SetRegistState(EMRegistState::Registing);

		GMsg::COM_ReqRegistSrv request;

		request.set_server_type((int)dnServer->GetServerType());

		if (uint64_t serverIndex = dnServer->ServerId())
		{
			request.set_server_id(serverIndex);
		}

		// pack data
		std::string binData;
		request.SerializeToString(&binData);
		
		// data alloc
		GMsg::COM_ResRegistSrv response;

		{
			auto taskGen = [](Message* msg) -> DNTask<Message*>
				{
					co_return msg;
				};
			auto dataChannel = taskGen(&response);
			
			uint32_t msgId = clientProxy->GetMsgId();
			clientProxy->AddMsg(msgId, &dataChannel);
			MessagePackAndSend(msgId, EMMsgDeal::Req, request.GetDescriptor()->full_name(), binData, clientProxy->GetChannel());
			co_await dataChannel;
			if (dataChannel.HasFlag(EMDNTaskFlag::Timeout))
			{
				dnServer->GetLogger()->Record(ELogLevel_Debug, "requst timeout! ");
			}

		}

		if (response.success())
		{
			dnServer->GetLogger()->Record(ELogLevel_Debug, "regist Server success! Rec index:{}", response.server_id());
			clientProxy->SetRegistState(EMRegistState::Registed);
			clientProxy->SetRegistType(response.server_type());
			dnServer->SetServerId(response.server_id());
		}
		else
		{
			dnServer->GetLogger()->Record(ELogLevel_Debug, "regist Server error!  ");
			// dnServer->IsRun() = false; //exit application
			clientProxy->SetRegistState(EMRegistState::None);
		}

		co_return;
	}

	// client request
	export void Msg_ReqRegistSrv(const DNSocketChannel::Ptr& channel, uint32_t msgId, const std::string& binMsg)
	{
		GMsg::d2L_ReqRegistSrv request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}

		LogicServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<LogicServerHelper>(EMSystemType::DNServer);
		
		dnServer->GetLogger()->Record(ELogLevel_Debug, "ip Reqregist: {}, {}", channel->peeraddr(), request.server_type());

		GMsg::COM_ResRegistSrv response;

		FinalExecute final([&response, msgId, channel](){
			std::string binData;
			response.SerializeToString(&binData);
			MessagePackAndSend(msgId, EMMsgDeal::Res, binData, channel);
		});

		RoomEntityManagerHelper::Ptr entityMan = dnServer->GetRoomEntityManager();

		EMServerType regType = (EMServerType)request.server_type();
		const std::string& ipPort = channel->localaddr();

		if (regType != EMServerType::DedicatedServer || ipPort.empty())
		{
			response.set_success(false);
		}

		//exist?
		if (RoomEntity::Ptr entity = channel->getContextPtr<RoomEntity>())
		{
			response.set_success(false);
		}

		else if (int serverId = request.server_id())
		{
			if (RoomEntity::Ptr entity = entityMan->GetEntity(serverId))
			{
				// wait destroy`s destroy
				if (uint64_t timerId = entity->TimerId())
				{
					entity->SetTimerId(0);
					entityMan->Timer()->killTimer(timerId);
				}

				// already connect
				if (const DNSocketChannel::Ptr& sock = entity->GetChannel())
				{
					response.set_success(false);
				}
				else
				{
					entity->SetChannel(channel);
					channel->setContextPtr(entity);

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
				entity->SetChannel(channel);

				channel->setContextPtr(entity);

				size_t pos = ipPort.find(":");
				entity->SetServerIp(ipPort.substr(0, pos));
				entity->SetServerPort(request.server_port());
			}
		}

		else if (RoomEntity::Ptr entity = entityMan->AddEntity(entityMan->GenRoomId(), request.map_id()))
		{
			size_t pos = ipPort.find(":");
			entity->SetServerIp(ipPort.substr(0, pos));
			entity->SetServerPort(request.server_port());

			dnServer->GetLogger()->Record(ELogLevel_Debug, "ds regist:{}:{}", entity->ServerIp(), entity->ServerPort());

			entity->SetChannel(channel);

			channel->setContextPtr(entity);

			response.set_success(true);
			response.set_server_id(entity->ID());
			response.set_server_type((uint8_t(dnServer->GetServerType())));
		}

	}

	export void Exe_RetChangeCtlSrv(const DNSocketChannel::Ptr& channel, const std::string& binMsg)
	{
		GMsg::COM_RetChangeCtlSrv request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}
		LogicServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<LogicServerHelper>(EMSystemType::DNServer);
		DNClientProxyHelper::Ptr clientProxy = dnServer->GetClientProxy();

		TickMainSpaceDll(clientProxy.get(), FUNCPLACE(DNClientProxy,RedirectClient),  request.server_port(), request.server_ip());
	}

	export void Exe_RetHeartbeat(const DNSocketChannel::Ptr& channel, const std::string& binMsg)
	{
		GMsg::COM_RetHeartbeat request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}
	}
}