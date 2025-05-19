module;
export module GlobalMessage:GlobalCommon;

import DNTask;
import FuncHelper;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import DNClientProxyHelper;
import DNServerProxyHelper;
import DNSocketProxy;
import DNServer;
import ServerEntity;
import ServerEntityManagerHelper;
import GlobalServerHelper;
import ECSW;

namespace GlobalMessage
{

	// client request
	export DNTaskVoid Evt_ReqRegistSrv(const DNServer::Ptr& server)
	{
		GlobalServerHelper::Ptr dnServer = server->GetSelf<GlobalServerHelper>();

		DNClientProxyHelper::Ptr clientProxy = dnServer->GetClientProxy();

		DNServerProxyHelper::Ptr serverProxy = dnServer->GetServerProxy();
		
		dnServer->GetLogger()->Record(ELogLevel_Debug, "Client:{}, port:{}", clientProxy->remote_host, clientProxy->remote_port);
		
		clientProxy->SetRegistState(EMRegistState::Registing);

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
				dnServer->GetLogger()->Record(ELogLevel_Debug, "requst timeout! ");
			}

		}

		if (response.success())
		{
			dnServer->GetLogger()->Record(ELogLevel_Debug, "regist Server success! Rec index:{}", response.server_id());
			clientProxy->SetRegistState(EMRegistState::Registed);
			clientProxy->SetRegistType(response.server_type());
			dnServer->SetServerId(response.server_id());
		}
		else
		{
			dnServer->GetLogger()->Record(ELogLevel_Debug, "regist Server error!  ");
			// dnServer->IsRun() = false; //exit application
			clientProxy->SetRegistState(EMRegistState::None);
		}


		co_return;
	}

	// client request
	export void Msg_ReqRegistSrv(const World::Ptr& world, const DNSocketProxy::Ptr& channel, uint32_t msgId, std::string binMsg)
	{
		GMsg::COM_ReqRegistSrv request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}

		GlobalServerHelper::Ptr dnServer = world->GetSystem<GlobalServerHelper>(EMSystemType::DNServer);
		
		dnServer->GetLogger()->Record(ELogLevel_Debug, "ip Reqregist: {}, {}", channel->peeraddr(), request.server_type());

		GMsg::COM_ResRegistSrv response;

		FinalExecute final([&response, msgId, channel, &dnServer](){
			std::string binData;
			response.SerializeToString(&binData);
			MessagePackAndSend(msgId, EMMsgDeal::Res, "", binData, channel);

			if (response.success())
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
			response.set_success(false);
		}

		//exist?
		else if (ServerEntity::Ptr entity = channel->getContextPtr<ServerEntity>())
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
					entity->SetTimerId(0);
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
					channel->setContextPtr(entity);

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

				channel->setContextPtr(entity);

				size_t pos = ipPort.find(":");
				entity->SetServerIp(ipPort.substr(0, pos));
				entity->SetServerPort(request.server_port());
			}

		}

		else if (ServerEntity::Ptr entity = entityMan->AddEntity(entityMan->GenServerId(), regType))
		{
			size_t pos = ipPort.find(":");
			entity->SetServerIp(ipPort.substr(0, pos));
			entity->SetServerPort(request.server_port());
			entity->SetSock(channel);

			channel->setContextPtr(entity);

			response.set_success(true);
			response.set_server_id(entity->ID());
			response.set_server_type((uint8_t(dnServer->GetServerType())));
		}

	}

	export void Exe_RetHeartbeat(const World::Ptr& world, const DNSocketProxy::Ptr& channel, std::string binMsg)
	{
		GMsg::COM_RetHeartbeat request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}
	}
}
