module;
export module GlobalMessage:GlobalCommon;

import DNTask;
import FuncHelper;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import DNClientProxyHelper;
import DNServerProxyHelper;

namespace GlobalMessage
{

	// client request
	export DNTaskVoid Evt_ReqRegistSrv(DNServer::WPtr dnServer)
	{
		DNServer::Ptr server = dnServer.lock();
		if (!server)
		{
			co_return;
		}

		DNClientProxyHelper::Ptr clientProxy = server->GetComponent<DNClientProxyHelper>(EMComponent::DNClientProxy);

		DNServerProxyHelper::Ptr serverProxy = server->GetComponent<DNServerProxyHelper>(EMComponent::DNServerProxy);
		
		SPidLogger.Record(ELogLevel_Debug, "Client:{}, port:{}", clientProxy->remote_host, clientProxy->remote_port);
		
		clientProxy->EMRegistState() = EMRegistState::Registing;

		GMsg::COM_ReqRegistSrv request;

		request.set_server_type((int)server->GetServerType());

		if (uint32_t serverId = server->ServerId())
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
				SPidLogger.Record(ELogLevel_Debug, "requst timeout! ");
			}

		}

		if (response.success())
		{
			SPidLogger.Record(ELogLevel_Debug, "regist Server success! Rec index:{}", response.server_id());
			clientProxy->EMRegistState() = EMRegistState::Registed;
			clientProxy->RegistType() = response.server_type();
			server->ServerId() = response.server_id();
		}
		else
		{
			SPidLogger.Record(ELogLevel_Debug, "regist Server error!  ");
			// dnServer->IsRun() = false; //exit application
			clientProxy->EMRegistState() = EMRegistState::None;
		}


		co_return;
	}

	// client request
	export void Msg_ReqRegistSrv(DNSocketProxy::Ptr channel, uint32_t msgId, std::string binMsg)
	{
		GMsg::COM_ReqRegistSrv request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}
		
		SPidLogger.Record(ELogLevel_Debug, "ip Reqregist: {}, {}", channel->peeraddr(), request.server_type());

		GMsg::COM_ResRegistSrv response;

		DNServer::Ptr dnServer = channel->GetWorld()->GetSystem<DNServer>();
	
		ServerEntityManagerHelper::Ptr entityMan = dnServer
			->GetComponent<ServerEntityManagerHelper>(EMComponent::ServerEntityManager);


		EMServerType regType = (EMServerType)request.server_type();

		const std::string& ipPort = channel->localaddr();

		if (regType < EMServerType::GateServer || regType > EMServerType::LogicServer || ipPort.empty())
		{
			response.set_success(false);
		}

		//exist?
		else if (ServerEntity::Ptr entity = channel->getContext<ServerEntity>())
		{
			response.set_success(false);
		}

		// take task to regist !
		else if (uint32_t serverId = request.server_id())
		{
			if (ServerEntity::Ptr entity = entityMan->GetEntity(serverId))
			{
				// wait destroy`s destroy
				if (uint64_t timerId = entity->TimerId())
				{
					entity->TimerId() = 0;
					entityMan->Timer()->killTimer(timerId);
				}

				// already connect
				if (const DNSocketProxy::Ptr& sock = entity->GetSock())
				{
					response.set_success(false);
				}
				else
				{
					entity->LinkNode() = nullptr;
					entity->SetSock(channel);
					channel->setContext(entity);

					response.set_success(true);

					// Re-enroll
					entityMan->MountEntity(regType, entity);
				}
			}
			else
			{
				response.set_success(true);
				response.set_server_id(serverId);
				response.set_server_type((uint8_t(dnServer->GetServerType())));

				entity = entityMan->AddEntity(serverId, regType);
				entity->SetSock(channel);

				channel->setContext(entity);

				size_t pos = ipPort.find(":");
				entity->ServerIp() = ipPort.substr(0, pos);
				entity->ServerPort() = request.server_port();
			}

		}

		else if (ServerEntity::Ptr entity = entityMan->AddEntity(entityMan->GenServerId(), regType))
		{
			size_t pos = ipPort.find(":");
			entity->ServerIp() = ipPort.substr(0, pos);
			entity->ServerPort() = request.server_port();
			entity->SetSock(channel);

			channel->setContext(entity);

			response.set_success(true);
			response.set_server_id(entity->ID());
			response.set_server_type((uint8_t(dnServer->GetServerType())));
		}

		std::string binData;
		response.SerializeToString(&binData);

		MessagePackAndSend(msgId, EMMsgDeal::Res, "", binData, channel);

		if (response.success())
		{
			dnServer->UpdateServerGroup();
		}

	}

	export void Exe_RetHeartbeat(DNSocketProxy::Ptr channel, std::string binMsg)
	{
		GMsg::COM_RetHeartbeat request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}
	}
}
