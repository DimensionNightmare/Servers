module;
export module GateMessage:GateCommon;

import DNTask;
import FuncHelper;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import ServerEntityManagerHelper;
import DNClientProxy;
import GateServerHelper;

namespace GateMessage
{

	void Evt_RetRegistChild(const DNServer::Ptr& server)
	{
		GateServerHelper::Ptr dnServer = server->GetSelf<GateServerHelper>();
		ServerEntityManagerHelper::Ptr entityMan = dnServer->GetServerEntityManager();
		DNClientProxyHelper::Ptr clientProxy = dnServer->GetClientProxy();

		GMsg::g2G_RetRegistChild request;

		request.set_server_id(dnServer->ServerId());

		auto AddChild = [&request](ServerEntity::Ptr serv)
			{
				GMsg::COM_ReqRegistSrv* child = request.add_childs();
				child->set_server_id(serv->ID());
				child->set_server_type((uint32_t)serv->GetServerType());
			};

		const std::list<ServerEntity::Ptr>& dbs = entityMan->GetEntitysByType(EMServerType::DatabaseServer);
		for (ServerEntity::Ptr serv : dbs)
		{
			AddChild(serv);
		}

		const std::list<ServerEntity::Ptr>& logics = entityMan->GetEntitysByType(EMServerType::LogicServer);
		for (ServerEntity::Ptr serv : logics)
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
	export DNTaskVoid Evt_ReqRegistSrv(const DNServer::Ptr& server)
	{
		GateServerHelper::Ptr dnServer = server->GetSelf<GateServerHelper>();

		DNClientProxyHelper::Ptr clientProxy = dnServer->GetClientProxy();
		DNServerProxyHelper::Ptr serverProxy = dnServer->GetServerProxy();
		
		dnServer->GetLogger()->Record(ELogLevel_Debug, "Client:{}, port:{}", clientProxy->remote_host, clientProxy->remote_port);
		
		clientProxy->EMRegistState() = EMRegistState::Registing;

		GMsg::COM_ReqRegistSrv request;

		request.set_server_type((int)dnServer->GetServerType());

		if (uint32_t serverId = dnServer->ServerId())
		{
			request.set_server_id(serverId);
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
				dnServer->GetLogger()->Record(ELogLevel_Debug, "requst timeout! ");
			}

		}

		if (response.success())
		{
			dnServer->GetLogger()->Record(ELogLevel_Debug, "regist Server success! Rec index:{}", response.server_id());
			clientProxy->EMRegistState() = EMRegistState::Registed;
			clientProxy->RegistType() = response.server_type();
			dnServer->ServerId() = response.server_id();

			Evt_RetRegistChild(server);
		}
		else
		{
			dnServer->GetLogger()->Record(ELogLevel_Debug, "regist Server error!");
			// dnServer->IsRun() = false; //exit application
			clientProxy->EMRegistState() = EMRegistState::None;
		}

		co_return;
	}

	// client request
	export void Msg_ReqRegistSrv(const DNSocketProxy::Ptr& channel, uint32_t msgId, std::string binMsg)
	{
		GMsg::COM_ReqRegistSrv request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}

		GateServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<GateServerHelper>(EMSystemType::DNServer);
		ServerEntityManagerHelper::Ptr entityMan = dnServer->GetServerEntityManager();

		dnServer->GetLogger()->Record(ELogLevel_Debug, "ip Reqregist: {}, {}", channel->peeraddr(), request.server_type());

		GMsg::COM_ResRegistSrv response;

		FinalExecute final([&response, msgId, channel](){
			std::string binData;
			response.SerializeToString(&binData);
			MessagePackAndSend(msgId, EMMsgDeal::Res, "", binData, channel);
		});

		
		EMServerType regType = (EMServerType)request.server_type();
		uint32_t serverId = request.server_id();

		const std::string& ipPort = channel->localaddr();

		if (regType < EMServerType::DatabaseServer || regType > EMServerType::LogicServer || ipPort.empty())
		{
			response.set_success(false);
		}

		//exist?
		if (ServerEntity::Ptr entity = channel->getContextPtr<ServerEntity>())
		{
			response.set_success(false);
		}

		else if (ServerEntity::Ptr entity = entityMan->AddEntity(serverId, regType))
		{
			size_t pos = ipPort.find(":");
			entity->ServerIp() = ipPort.substr(0, pos);
			entity->ServerPort() = request.server_port();
			entity->SetSock(channel);

			channel->setContextPtr(entity);

			response.set_success(true);
			response.set_server_id(entity->ID());
			response.set_server_type((uint8_t(dnServer->GetServerType())));
		}
		else
		{
			abort();
		}

		if (response.success())
		{
			// up to Global
			GMsg::g2G_RetRegistSrv request;
			request.set_is_regist(true);
			request.set_server_id(serverId);

			std::string binData;
			request.SerializeToString(&binData);
			
			DNClientProxyHelper::Ptr clientProxy = dnServer->GetClientProxy();

			MessagePackAndSend(0, EMMsgDeal::Ret, request.GetDescriptor()->full_name(), binData, clientProxy->GetChannel());
		}
	}

	export void Exe_RetHeartbeat(const DNSocketProxy::Ptr& channel, std::string binMsg)
	{
		GMsg::COM_RetHeartbeat request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}
	}
}
