export module LogicServerMessage:LogicCommon;

import FuncHelper;
import LogicServerHelper;
import Server;
import Task;
import ThirdParty.PbGen;
import Logger;
import MessagePack;
import LogicServerMessage;

namespace MsgHandleRegister
{

	// client request
	HandleClientRegistry Evt_ReqRegistSrv([](Server::Ptr server) -> TaskVoid
	{
		LogicServerHelper::Ptr dnServer = server->GetSelf<LogicServerHelper>();

		ClientProxyHelper::Ptr clientProxy = dnServer->GetClientProxy();

		LoggerPrint::Log(server, ELogLevel_Debug, "Client:{}, port:{}", clientProxy->remote_host, clientProxy->remote_port);

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
			LoggerPrint::Log(server, response.errorcode());
			// dnServer->IsRun() = false; //exit application
			clientProxy->SetRegistState(EMRegistState::None);
		}

		co_return;
	});

	HandleRegistry<GMsg::d2L_ReqRegistSrv, GMsg::COM_ResRegistSrv, EMMsgDeal::Req> Msg_ReqRegistSrv(
				[](auto request, auto response, SocketChannel::Ptr channel)
	{

		LogicServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<LogicServerHelper>(EMSystemType::Server);

		LoggerPrint::Log(channel, ELogLevel_Debug, "ip Reqregist: {}, {}", channel->peeraddr(), request->servertype());

		RoomEntityManagerHelper::Ptr entityMan = dnServer->GetRoomEntityManager();

		EMServerType regType = (EMServerType)request->servertype();
		const std::string& ipPort = channel->localaddr();

		if (regType != EMServerType::DedicatedServer || ipPort.empty())
		{
			response->set_errorcode(EL10nCode_RegistServerTypeError);
		}

		//exist?
		if (RoomEntityHelper::Ptr entity = channel->getContextPtr<RoomEntityHelper>())
		{
			response->set_errorcode(EL10nCode_RegistServerChannelExist);
		}

		else if (request->ispull())
		{
			if (entity = entityMan->GetEntity(request->serverid()))
			{
				// wait destroy`s destroy
				if (size_t timerId = entity->TimerId())
				{
					entity->SetTimerId(0);
					entityMan->GetTimer()->KillTimer(timerId);
				}

				// already connect
				if (SocketChannel::Ptr sock = entity->GetChannel())
				{
					response->set_errorcode(EL10nCode_PullServerReqRegistAlready);
				}
				else
				{
					entity->SetChannel(channel);
					channel->setContextPtr(entity);
					// entity->SetLinkNode(nullptr);

					size_t pos = ipPort.find(":");
					entity->SetServerIp(ipPort.substr(0, pos));
					entity->SetServerPort(request->serverport());

					// Re-enroll
					entityMan->MountEntity(entity);

					response->set_retservertype(std::to_underlying(dnServer->GetServerType()));
				}
			}
			else
			{
				response->set_errorcode(EL10nCode_PullServerTimeout);
			}
		}

		else if (entity = entityMan->AddEntity(request->mapid()))
		{
			size_t pos = ipPort.find(":");
			entity->SetServerIp(ipPort.substr(0, pos));
			entity->SetServerPort(request->serverport());

			LoggerPrint::Log(channel, ELogLevel_Debug, "ds regist:{}:{}", entity->ServerIp(), entity->ServerPort());

			entity->SetChannel(channel);

			channel->setContextPtr(entity);

			response->set_retservertype(std::to_underlying(dnServer->GetServerType()));
		}
	});

	HandleRegistry<GMsg::COM_RetChangeCtlSrv, void, EMMsgDeal::Ret> Exe_RetChangeCtlSrv(
				[](auto request, SocketChannel::Ptr channel)
	{
		
		LogicServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<LogicServerHelper>(EMSystemType::Server);
		ClientProxyHelper::Ptr clientProxy = dnServer->GetClientProxy();

		clientProxy->RedirectClient(request->serverport(), request->serverip());
	});

	HandleRegistry<GMsg::COM_RetHeartbeat, void, EMMsgDeal::Ret> Exe_RetHeartbeat(
				[](auto request, SocketChannel::Ptr channel)
	{
	});

}