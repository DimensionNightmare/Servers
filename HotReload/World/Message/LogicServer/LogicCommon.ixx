export module LogicServerMessage:LogicCommon;

import FuncHelper;
import LogicServerHelper;
import Server;
import Task;
import ThirdParty.PbGen;
import Logger;
import LogicServerMessage;

namespace MsgHandleRegister
{

	// client request
	HandleClientRegistry Evt_ReqRegistSrv = [](Server::Ptr server) -> TaskVoid
	{
		LogicServerHelper::CVPtr dnServer = server->GetSelf<LogicServerHelper>();

		ClientProxyHelper::CVPtr clientProxy = dnServer->GetClientProxy();

		ServerProxyHelper::CVPtr serverProxy = dnServer->GetServerProxy();

		World::CVPtr world = dnServer->GetWorld();

		LoggerPrint::Log(world, ELogLevel_Debug, "Client:{}, port:{}", clientProxy->remote_host, clientProxy->remote_port);

		clientProxy->SetRegistState(EMRegistState::Registing);

		GMsg::COM_ReqRegistSrv request;

		request.set_serverid(dnServer->ID());
		request.set_servertype(std::to_underlying(dnServer->GetServerType()));

		if (dnServer->IsPullServer())
		{
			request.set_ispull(true);
		}

		request.set_serverport(serverProxy->port);
		
		// data alloc
		GMsg::COM_ResRegistSrv response;

		bool success = co_await clientProxy->AddMsg(EMMsgDeal::Req, &request, &response);
		
		if (!success)
		{
			response.set_errorcode(EL10nCode_ReqRegistTimeout);
		}

		if (response.errorcode() == EL10nCode_None)
		{
			clientProxy->SetRegistState(EMRegistState::Registed);
			clientProxy->SetRegistType(response.retservertype());
		}
		else
		{
			LoggerPrint::Log(world, response.errorcode());
			// dnServer->IsRun() = false; //exit application
			clientProxy->SetRegistState(EMRegistState::None);
		}

		co_return;
	};

	HandleRegistry<GMsg::d2L_ReqRegistSrv, GMsg::COM_ResRegistSrv, EMMsgDeal::Req> Msg_ReqRegistSrv =
				[](World::Ptr world, SocketChannel::Ptr channel, auto request, auto response)
	{
		
		LogicServerHelper::CVPtr dnServer = world->GetSystem<LogicServerHelper>(EMSystemType::Server);

		LoggerPrint::Log(world, ELogLevel_Debug, "ip Reqregist: {}, {}", channel->peeraddr(), request->servertype());

		RoomEntityManagerHelper::CVPtr entityMan = dnServer->GetRoomEntityManager();

		EMServerType regType = static_cast<EMServerType>(request->servertype());
		const std::string& ipPort = channel->localaddr();

		if (regType != EMServerType::DedicatedServer || ipPort.empty())
		{
			response->set_errorcode(EL10nCode_RegistServerTypeError);
		}

		RoomEntityHelper::Ptr entity;

		//exist?
		if (entity = channel->GetEntity<RoomEntityHelper>())
		{
			response->set_errorcode(EL10nCode_RegistServerChannelExist);
		}

		else if (request->ispull())
		{
			if (entity = entityMan->GetEntity(request->serverid()))
			{
				// wait destroy`s destroy
				if (size_t timerId = entity->GetTimerId())
				{
					entity->SetTimerId(0);
					entityMan->GetTimer()->KillTimer(timerId);
				}

				// already connect
				if (SocketChannel::CVPtr sock = entity->GetChannel())
				{
					response->set_errorcode(EL10nCode_PullServerReqRegistAlready);
				}
				else
				{
					entity->SetChannel(channel);
					channel->SetEntity(entity);
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

			LoggerPrint::Log(world, ELogLevel_Debug, "ds regist:{}:{}, mapId:{}", entity->GetServerIp(), entity->GetServerPort(), request->mapid());

			entity->SetChannel(channel);

			channel->SetEntity(entity);

			response->set_retservertype(std::to_underlying(dnServer->GetServerType()));
		}
	};

	HandleRegistry<GMsg::COM_RetChangeCtlSrv, void, EMMsgDeal::Ret> Exe_RetChangeCtlSrv =
				[](World::Ptr world, SocketChannel::Ptr channel, auto request)
	{
		
		LogicServerHelper::CVPtr dnServer = world->GetSystem<LogicServerHelper>(EMSystemType::Server);
		ClientProxyHelper::CVPtr clientProxy = dnServer->GetClientProxy();

		clientProxy->RedirectClient(request->serverport(), request->serverip());
	};

	HandleRegistry<GMsg::COM_RetHeartbeat, void, EMMsgDeal::Ret> Exe_RetHeartbeat =
				[](World::Ptr world, SocketChannel::Ptr channel, auto request)
	{
	};

}