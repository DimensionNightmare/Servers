module;
export module LogicServerMessage:LogicCommon;

import DllUtils;
import FuncHelper;
import LogicServerHelper;
import DNServer;
import DNTask;


#define FUNCPLACE(class, func) &class::func, #class"_"#func

namespace LogicServerMessage
{

	// client request
	export DNTaskVoid Evt_ReqRegistSrv(DNServer::CVPtr server)
	{
		LogicServerHelper::CVPtr dnServer = server->GetSelf<LogicServerHelper>();

		DNClientProxyHelper::CVPtr clientProxy = dnServer->GetClientProxy();
		
		dnServer->GetLogger()->Record(ELogLevel_Debug, "Client:{}, port:{}", clientProxy->remote_host, clientProxy->remote_port);
		
		clientProxy->SetRegistState(EMRegistState::Registing);

		GMsg::COM_ReqRegistSrv request;

		request.set_server_id(dnServer->ID());
		request.set_server_type((int)dnServer->GetServerType());

		if (dnServer->IsPullServer())
		{
			request.set_is_pull(true);
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
				response.set_error_code(EL10nCode_ReqRegistTimeout);
			}

		}

		if (response.error_code() == EL10nCode_None)
		{
			clientProxy->SetRegistState(EMRegistState::Registed);
			clientProxy->SetRegistType(response.ret_server_type());
		}
		else
		{
			dnServer->GetLogger()->Record(response.error_code());
			// dnServer->IsRun() = false; //exit application
			clientProxy->SetRegistState(EMRegistState::None);
		}

		co_return;
	}

	// client request
	export void Msg_ReqRegistSrv(DNSocketChannel::CVPtr channel, uint32_t msgId, const std::string& binMsg)
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
			response.set_error_code(EL10nCode_RegistServerTypeError);
		}

		//exist?
		if (RoomEntity::CVPtr entity = channel->getContextPtr<RoomEntity>())
		{
			response.set_error_code(EL10nCode_RegistServerChannelExist);
		}

		else if (request.is_pull())
		{
			if (RoomEntityHelper::CVPtr entity = entityMan->GetEntity(request.server_id()))
			{
				// wait destroy`s destroy
				if (uint64_t timerId = entity->TimerId())
				{
					entity->SetTimerId(0);
					entityMan->Timer()->killTimer(timerId);
				}

				// already connect
				if (DNSocketChannel::CVPtr sock = entity->GetChannel())
				{
					response.set_error_code(EL10nCode_PullServerReqRegistAlready);
				}
				else
				{
					entity->SetChannel(channel);
					channel->setContextPtr(entity);
					// entity->SetLinkNode(nullptr);

					size_t pos = ipPort.find(":");
					entity->SetServerIp(ipPort.substr(0, pos));
					entity->SetServerPort(request.server_port());

					// Re-enroll
					entityMan->MountEntity(entity);

					response.set_ret_server_type(static_cast<uint8_t>(dnServer->GetServerType()));
				}
			}
			else
			{
				response.set_error_code(EL10nCode_PullServerTimeout);
			}
		}

		else if (RoomEntityHelper::CVPtr entity = entityMan->AddEntity(entityMan->GenRoomId(), request.map_id()))
		{
			size_t pos = ipPort.find(":");
			entity->SetServerIp(ipPort.substr(0, pos));
			entity->SetServerPort(request.server_port());

			dnServer->GetLogger()->Record(ELogLevel_Debug, "ds regist:{}:{}", entity->ServerIp(), entity->ServerPort());

			entity->SetChannel(channel);

			channel->setContextPtr(entity);

			response.set_ret_server_type(static_cast<uint8_t>(dnServer->GetServerType()));
		}

	}

	export void Exe_RetChangeCtlSrv(DNSocketChannel::CVPtr channel, const std::string& binMsg)
	{
		GMsg::COM_RetChangeCtlSrv request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}
		LogicServerHelper::CVPtr dnServer = channel->GetWorld()->GetSystem<LogicServerHelper>(EMSystemType::DNServer);
		DNClientProxyHelper::CVPtr clientProxy = dnServer->GetClientProxy();

		TickMainSpaceDll(clientProxy.get(), FUNCPLACE(DNClientProxy,RedirectClient),  request.server_port(), request.server_ip());
	}

	export void Exe_RetHeartbeat(DNSocketChannel::CVPtr channel, const std::string& binMsg)
	{
		GMsg::COM_RetHeartbeat request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}
	}
}