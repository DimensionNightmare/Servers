module;
export module GateServerMessage:GateCommon;

import GateServerHelper;
import FuncHelper;
import Server;


namespace GateServerMessage
{

	void Evt_RetRegistChild(Server::CVPtr server)
	{
		GateServerHelper::Ptr dnServer = server->GetSelf<GateServerHelper>();
		ServerEntityManagerHelper::Ptr entityMan = dnServer->GetServerEntityManager();
		ClientProxyHelper::Ptr clientProxy = dnServer->GetClientProxy();

		GMsg::g2G_RetRegistChild request;

		request.set_server_id(dnServer->ID());

		auto AddChild = [&request](ServerEntity::CVPtr serv)
			{
				GMsg::COM_ReqRegistSrv* child = request.add_childs();
				child->set_server_id(serv->ID());
				child->set_server_type((uint32_t)serv->GetServerType());
			};

		const std::list<ServerEntity::Ptr>& dbs = entityMan->GetEntitysByType(EMServerType::DatabaseServer);
		for (ServerEntity::CVPtr serv : dbs)
		{
			AddChild(serv);
		}

		const std::list<ServerEntity::Ptr>& logics = entityMan->GetEntitysByType(EMServerType::LogicServer);
		for (ServerEntity::CVPtr serv : logics)
		{
			AddChild(serv);
		}

		if( !request.childs_size())
		{
			return;
		}

		// pack data
		std::string binData;
		request.SerializeToString(&binData);
		MessagePackAndSend(0, EMMsgDeal::Ret, request.GetDescriptor()->full_name(), binData, clientProxy->GetChannel());
	}

	// self request
	export TaskVoid Evt_ReqRegistSrv(Server::CVPtr server)
	{
		GateServerHelper::Ptr dnServer = server->GetSelf<GateServerHelper>();

		ClientProxyHelper::Ptr clientProxy = dnServer->GetClientProxy();
		ServerProxyHelper::Ptr serverProxy = dnServer->GetServerProxy();
		
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
				response.set_error_code(EL10nCode_ReqRegistTimeout);
			}

		}

		if (response.error_code() == EL10nCode_None)
		{
			clientProxy->SetRegistState(EMRegistState::Registed);
			clientProxy->SetRegistType(response.ret_server_type());

			Evt_RetRegistChild(dnServer);
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
	export void Msg_ReqRegistSrv(SocketChannel::CVPtr channel, uint32_t msgId, const std::string& binMsg)
	{
		GMsg::COM_ReqRegistSrv request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}

		GateServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<GateServerHelper>(EMSystemType::Server);
		ServerEntityManagerHelper::Ptr entityMan = dnServer->GetServerEntityManager();

		dnServer->GetLogger()->Record(ELogLevel_Debug, "ip Reqregist: {}, {}", channel->peeraddr(), request.server_type());

		GMsg::COM_ResRegistSrv response;

		FinalExecute final([&response, msgId, channel](){
			std::string binData;
			response.SerializeToString(&binData);
			MessagePackAndSend(msgId, EMMsgDeal::Res, binData, channel);
		});

		
		EMServerType regType = (EMServerType)request.server_type();
		uint64_t serverId = request.server_id();

		const std::string& ipPort = channel->localaddr();

		if (regType < EMServerType::DatabaseServer || regType > EMServerType::LogicServer || ipPort.empty())
		{
			response.set_error_code(EL10nCode_RegistServerTypeError);
		}

		//exist?
		if (ServerEntityHelper::Ptr entity = channel->getContextPtr<ServerEntityHelper>())
		{
			response.set_error_code(EL10nCode_RegistServerChannelExist);
		}

		else if (entity = entityMan->AddEntity(serverId, regType))
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

		if (response.error_code() == EL10nCode_None)
		{
			// up to Global
			GMsg::g2G_RetRegistSrv request;
			request.set_is_regist(true);
			request.set_server_id(serverId);

			std::string binData;
			request.SerializeToString(&binData);
			
			ClientProxyHelper::Ptr clientProxy = dnServer->GetClientProxy();

			MessagePackAndSend(0, EMMsgDeal::Ret, request.GetDescriptor()->full_name(), binData, clientProxy->GetChannel());
		}
	}

	export void Exe_RetHeartbeat(SocketChannel::CVPtr channel, const std::string& binMsg)
	{
		GMsg::COM_RetHeartbeat request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}
	}
}
