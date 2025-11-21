export module GateServerMessage:GateCommon;

import GateServerHelper;
import FuncHelper;
import Server;
import ThirdParty.PbGen;
import Logger;
import GateServerMessage;

namespace MsgHandleRegister
{

	void Evt_RetRegistChild(Server::CVPtr server)
	{
		GateServerHelper::CVPtr dnServer = server->GetSelf<GateServerHelper>();
		ServerEntityManagerHelper::CVPtr entityMan = dnServer->GetServerEntityManager();
		ClientProxyHelper::CVPtr clientProxy = dnServer->GetClientProxy();

		GMsg::g2G_RetRegistChild request;

		request.set_serverid(dnServer->ID());

		auto AddChild = [&request](ServerEntity::CVPtr serv)
			{
				GMsg::COM_ReqRegistSrv* child = request.add_childs();
				child->set_serverid(serv->ID());
				child->set_servertype((uint32_t)serv->GetServerType());
			};

		const std::list<ServerEntity::Ptr>& dbs = entityMan->GetEntitysByType(EMServerType::DatabaseServer);
		for (const auto& serv : dbs)
		{
			AddChild(serv);
		}

		const std::list<ServerEntity::Ptr>& logics = entityMan->GetEntitysByType(EMServerType::LogicServer);
		for (const auto& serv : logics)
		{
			AddChild(serv);
		}

		if( !request.childs_size())
		{
			return;
		}

		clientProxy->AddMsg(EMMsgDeal::Ret, &request).Resume();
	}

	// self request
	HandleClientRegistry Evt_ReqRegistSrv = [](Server::Ptr server) -> TaskVoid
	{
		GateServerHelper::CVPtr dnServer = server->GetSelf<GateServerHelper>();

		ClientProxyHelper::CVPtr clientProxy = dnServer->GetClientProxy();
		ServerProxyHelper::CVPtr serverProxy = dnServer->GetServerProxy();

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

			Evt_RetRegistChild(dnServer);
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
				[](auto request, auto response, SocketChannel::CVPtr channel)
	{
		
		GateServerHelper::CVPtr server = channel->GetWorld()->GetSystem<GateServerHelper>(EMSystemType::Server);
		ServerEntityManagerHelper::CVPtr entityMan = server->GetServerEntityManager();

		LoggerPrint::Log(server, ELogLevel_Debug, "ip Reqregist: {}, {}", channel->peeraddr(), request->servertype());

		EMServerType regType = static_cast<EMServerType>(request->servertype());
		size_t serverId = request->serverid();

		const std::string& ipPort = channel->localaddr();

		if (regType < EMServerType::DatabaseServer || regType > EMServerType::LogicServer || ipPort.empty())
		{
			response->set_errorcode(EL10nCode_RegistServerTypeError);
		}

		ServerEntityHelper::Ptr entity;

		//exist?
		if (entity = channel->getContextPtr<ServerEntityHelper>())
		{
			response->set_errorcode(EL10nCode_RegistServerChannelExist);
		}

		else if (entity = entityMan->AddEntity(serverId, regType))
		{
			size_t pos = ipPort.find(":");
			entity->SetServerIp(ipPort.substr(0, pos));
			entity->SetServerPort(request->serverport());
			entity->SetChannel(channel);

			channel->SetEntity(entity);

			response->set_retservertype(std::to_underlying(server->GetServerType()));
		}
		else
		{
			response->set_errorcode(EL10nCode_UnkonwOpreator);
		}

		if (response->errorcode() == EL10nCode_None)
		{
			// up to Global
			GMsg::g2G_RetRegistSrv request;
			request.set_isregist(true);
			request.set_serverid(serverId);

			ClientProxyHelper::CVPtr clientProxy = server->GetClientProxy();

			clientProxy->AddMsg(EMMsgDeal::Ret, &request).Resume();
		}
	};

	HandleRegistry<GMsg::COM_RetHeartbeat, void, EMMsgDeal::Ret> Exe_RetHeartbeat =
				[](auto request, SocketChannel::CVPtr channel)
	{
	};
}
