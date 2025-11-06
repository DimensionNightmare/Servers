export module GlobalServerMessage:GlobalCommon;

import FuncHelper;
import GlobalServerHelper;
import Server;
import Task;
import ThirdParty.PbGen;
import Logger;
import GlobalServerMessage;

namespace MsgHandleRegister
{

	// client request
	HandleClientRegistry Evt_ReqRegistSrv = [](Server::Ptr server)->TaskVoid
	{
		GlobalServerHelper::Ptr dnServer = server->GetSelf<GlobalServerHelper>();

		ClientProxyHelper::Ptr clientProxy = dnServer->GetClientProxy();

		ServerProxyHelper::Ptr serverProxy = dnServer->GetServerProxy();

		LoggerPrint::Log(server, ELogLevel_Debug, "Client:{}, port:{}", clientProxy->remote_host, clientProxy->remote_port);

		clientProxy->SetRegistState(EMRegistState::Registing);

		GMsg::COM_ReqRegistSrv request;

		request.set_serverid(dnServer->ID());
		request.set_servertype((int)dnServer->GetServerType());

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
			LoggerPrint::Log(server, response.errorcode());
			// dnServer->IsRun() = false; //exit application
			clientProxy->SetRegistState(EMRegistState::None);
		}


		co_return;
	};

	HandleRegistry<GMsg::COM_ReqRegistSrv, GMsg::COM_ResRegistSrv, EMMsgDeal::Req> Msg_ReqRegistSrv =
				[](auto request, auto response, const SocketChannel::Ptr& channel, auto reply)
	{
		
		GlobalServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<GlobalServerHelper>(EMSystemType::Server);

		LoggerPrint::Log(channel, ELogLevel_Debug, "ip Reqregist: {}, {}", channel->peeraddr(), request->servertype());

		ServerEntityManagerHelper::Ptr entityMan = dnServer
			->GetComponent<ServerEntityManagerHelper>(EMComponentType::ServerEntityManager);


		EMServerType regType = static_cast<EMServerType>(request->servertype());

		const std::string& ipPort = channel->localaddr();

		if (regType < EMServerType::GateServer || regType > EMServerType::LogicServer || ipPort.empty())
		{
			response->set_errorcode(EL10nCode_RegistServerTypeError);
		}

		//exist?
		else if (ServerEntityHelper::Ptr entity = channel->getContextPtr<ServerEntityHelper>())
		{
			response->set_errorcode(EL10nCode_RegistServerChannelExist);
		}

		// take task to regist !
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
				if (const SocketChannel::Ptr& sock = entity->GetChannel())
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

		else if (entity = entityMan->AddEntity(request->serverid(), regType))
		{
			size_t pos = ipPort.find(":");
			entity->SetServerIp(ipPort.substr(0, pos));
			entity->SetServerPort(request->serverport());
			entity->SetChannel(channel);

			channel->setContextPtr(entity);

			response->set_retservertype(std::to_underlying(dnServer->GetServerType()));
		}
		else
		{
			response->set_errorcode(EL10nCode_UnkonwOpreator);
		}

		reply();
		
		if (response->errorcode() == EL10nCode_None)
		{
			channel->GetWorld()->PostTask([dnServer](){dnServer->UpdateServerGroup();});
		}

	};

	HandleRegistry<GMsg::COM_RetHeartbeat, void, EMMsgDeal::Ret> Exe_RetHeartbeat =
				[](auto request, const SocketChannel::Ptr& channel)
	{

	};
}
