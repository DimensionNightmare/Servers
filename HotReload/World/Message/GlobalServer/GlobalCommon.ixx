module;
export module GlobalServerMessage:GlobalCommon;

import FuncHelper;
import GlobalServerHelper;
import DNServer;
import DNTask;


namespace GlobalServerMessage
{

	// client request
	export DNTaskVoid Evt_ReqRegistSrv(DNServer::CVPtr server)
	{
		GlobalServerHelper::Ptr dnServer = server->GetSelf<GlobalServerHelper>();

		DNClientProxyHelper::Ptr clientProxy = dnServer->GetClientProxy();

		DNServerProxyHelper::Ptr serverProxy = dnServer->GetServerProxy();
		
		dnServer->GetLogger()->Record(ELogLevel_Debug, "Client:{}, port:{}", clientProxy->remote_host, clientProxy->remote_port);
		
		clientProxy->SetRegistState(EMRegistState::Registing);

		GMsg::COM_ReqRegistSrv request;

		request.set_server_id(dnServer->ID());
		request.set_server_type((int)dnServer->GetServerType());

		if (dnServer->IsPullServer())
		{
			request.set_is_pull(true);
		}

		request.set_server_port(serverProxy->port);

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
		GMsg::COM_ReqRegistSrv request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}

		GlobalServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<GlobalServerHelper>(EMSystemType::DNServer);
		
		dnServer->GetLogger()->Record(ELogLevel_Debug, "ip Reqregist: {}, {}", channel->peeraddr(), request.server_type());

		GMsg::COM_ResRegistSrv response;

		FinalExecute final([&response, msgId, channel, &dnServer](){
			std::string binData;
			response.SerializeToString(&binData);
			MessagePackAndSend(msgId, EMMsgDeal::Res, binData, channel);

			if (response.error_code() == EL10nCode_None)
			{
				dnServer->UpdateServerGroup();
			}
		});

		ServerEntityManagerHelper::Ptr entityMan = dnServer
			->GetComponent<ServerEntityManagerHelper>(EMComponentType::ServerEntityManager);


		EMServerType regType = (EMServerType)request.server_type();

		const std::string& ipPort = channel->localaddr();

		if (regType < EMServerType::GateServer || regType > EMServerType::LogicServer || ipPort.empty())
		{
			response.set_error_code(EL10nCode_RegistServerTypeError);
		}

		//exist?
		else if (ServerEntityHelper::Ptr entity = channel->getContextPtr<ServerEntityHelper>())
		{
			response.set_error_code(EL10nCode_RegistServerChannelExist);
		}

		// take task to regist !
		else if (request.is_pull())
		{
			if (entity = entityMan->GetEntity(request.server_id()))
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

		else if (entity = entityMan->AddEntity(request.server_id(), regType))
		{
			size_t pos = ipPort.find(":");
			entity->SetServerIp(ipPort.substr(0, pos));
			entity->SetServerPort(request.server_port());
			entity->SetChannel(channel);

			channel->setContextPtr(entity);

			response.set_ret_server_type(static_cast<uint8_t>(dnServer->GetServerType()));
		}
		else
		{
			response.set_error_code(EL10nCode_UnkonwOpreator);
		}

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
