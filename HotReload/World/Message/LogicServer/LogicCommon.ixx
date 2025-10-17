export module LogicServerMessage:LogicCommon;

import FuncHelper;
import LogicServerHelper;
import Server;
import Task;
import ThirdParty.PbGen;
import Logger;

export namespace LogicServerMessage
{

	// client request
	TaskVoid Evt_ReqRegistSrv(Server::CVPtr server)
	{
		LogicServerHelper::CVPtr dnServer = server->GetSelf<LogicServerHelper>();

		ClientProxyHelper::CVPtr clientProxy = dnServer->GetClientProxy();

		LoggerPrint::Log(dnServer, ELogLevel_Debug, "Client:{}, port:{}", clientProxy->remote_host, clientProxy->remote_port);

		clientProxy->SetRegistState(EMRegistState::Registing);

		GMsg::COM_ReqRegistSrv request;

		request.set_serverid(dnServer->ID());
		request.set_servertype((int)dnServer->GetServerType());

		if (dnServer->IsPullServer())
		{
			request.set_ispull(true);
		}

		// pack data
		std::string binData;
		request.SerializeToString(&binData);
		
		// data alloc
		GMsg::COM_ResRegistSrv response;

		{
			auto taskGen = [](Message* msg) -> Task<Message*>
				{
					co_return msg;
				};
			auto dataChannel = taskGen(&response);
			
			uint32_t msgId = clientProxy->GetMsgId();
			clientProxy->AddMsg(msgId, &dataChannel);
			MessagePackAndSend(msgId, EMMsgDeal::Req, request.GetDescriptor()->full_name(), binData, clientProxy->GetChannel());
			co_await dataChannel;
			if (dataChannel.HasFlag(EMTaskFlag::Timeout))
			{
				response.set_errorcode(EL10nCode_ReqRegistTimeout);
			}

		}

		if (response.errorcode() == EL10nCode_None)
		{
			clientProxy->SetRegistState(EMRegistState::Registed);
			clientProxy->SetRegistType(response.retservertype());
		}
		else
		{
			LoggerPrint::Log(dnServer, response.errorcode());
			// dnServer->IsRun() = false; //exit application
			clientProxy->SetRegistState(EMRegistState::None);
		}

		co_return;
	}

	// client request
	void Msg_ReqRegistSrv(SocketChannel::CVPtr channel, uint32_t msgId, const std::string& binMsg)
	{
		GMsg::d2L_ReqRegistSrv request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}

		LogicServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<LogicServerHelper>(EMSystemType::Server);

		LoggerPrint::Log(channel, ELogLevel_Debug, "ip Reqregist: {}, {}", channel->peeraddr(), request.servertype());

		GMsg::COM_ResRegistSrv response;

		FinalExecute final([&response, msgId, channel](){
			std::string binData;
			response.SerializeToString(&binData);
			MessagePackAndSend(msgId, EMMsgDeal::Res, binData, channel);
		});

		RoomEntityManagerHelper::Ptr entityMan = dnServer->GetRoomEntityManager();

		EMServerType regType = (EMServerType)request.servertype();
		const std::string& ipPort = channel->localaddr();

		if (regType != EMServerType::DedicatedServer || ipPort.empty())
		{
			response.set_errorcode(EL10nCode_RegistServerTypeError);
		}

		//exist?
		if (RoomEntityHelper::Ptr entity = channel->getContextPtr<RoomEntityHelper>())
		{
			response.set_errorcode(EL10nCode_RegistServerChannelExist);
		}

		else if (request.ispull())
		{
			if (entity = entityMan->GetEntity(request.serverid()))
			{
				// wait destroy`s destroy
				if (uint64_t timerId = entity->TimerId())
				{
					entity->SetTimerId(0);
					entityMan->GetTimer()->KillTimer(timerId);
				}

				// already connect
				if (SocketChannel::CVPtr sock = entity->GetChannel())
				{
					response.set_errorcode(EL10nCode_PullServerReqRegistAlready);
				}
				else
				{
					entity->SetChannel(channel);
					channel->setContextPtr(entity);
					// entity->SetLinkNode(nullptr);

					size_t pos = ipPort.find(":");
					entity->SetServerIp(ipPort.substr(0, pos));
					entity->SetServerPort(request.serverport());

					// Re-enroll
					entityMan->MountEntity(entity);

					response.set_retservertype(static_cast<uint8_t>(dnServer->GetServerType()));
				}
			}
			else
			{
				response.set_errorcode(EL10nCode_PullServerTimeout);
			}
		}

		else if (entity = entityMan->AddEntity(request.mapid()))
		{
			size_t pos = ipPort.find(":");
			entity->SetServerIp(ipPort.substr(0, pos));
			entity->SetServerPort(request.serverport());

			LoggerPrint::Log(channel, ELogLevel_Debug, "ds regist:{}:{}", entity->ServerIp(), entity->ServerPort());

			entity->SetChannel(channel);

			channel->setContextPtr(entity);

			response.set_retservertype(static_cast<uint8_t>(dnServer->GetServerType()));
		}

	}

	void Exe_RetChangeCtlSrv(SocketChannel::CVPtr channel, const std::string& binMsg)
	{
		GMsg::COM_RetChangeCtlSrv request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}
		LogicServerHelper::CVPtr dnServer = channel->GetWorld()->GetSystem<LogicServerHelper>(EMSystemType::Server);
		ClientProxyHelper::CVPtr clientProxy = dnServer->GetClientProxy();

		clientProxy->RedirectClient(request.serverport(), request.serverip());
	}

	void Exe_RetHeartbeat(SocketChannel::CVPtr channel, const std::string& binMsg)
	{
		GMsg::COM_RetHeartbeat request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}
	}
}