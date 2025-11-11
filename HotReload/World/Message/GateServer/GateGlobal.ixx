export module GateServerMessage:GateGlobal;

import GateServerHelper;
import ProxyEntityHelper;
import StrUtils;
import FuncHelper;
import ThirdParty.PbGen;
import Logger;
import GateServerMessage;

namespace MsgHandleRegister
{

	HandleRegistry<GMsg::A2g_ReqAuthAccount, GMsg::g2A_ResAuthAccount, EMMsgDeal::Req> Exe_ReqUserToken =
				[](auto request, auto response, const SocketChannel::Ptr& channel)
	{

		GateServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<GateServerHelper>(EMSystemType::Server);
		ProxyEntityManagerHelper::Ptr entityMan = dnServer->GetProxyEntityManager();
		ProxyEntityHelper::Ptr entity = entityMan->GetEntity(request->accountid());
		if (entity)
		{
			//exit
			if (const SocketChannel::Ptr& online = entity->GetChannel())
			{
				// kick channel
				GMsg::S2C_RetAccountReplace notify_request;
				notify_request.set_serverip(request->serverip());
				
				MessagePackAndSend(0, EMMsgDeal::Ret, &notify_request, online);

				//kick socket
				online->deleteContextPtr();
				online->close();


				//kick game
				if (size_t serverId = entity->GetRecordServerId())
				{
					LoggerPrint::Log(channel, ELogLevel_Debug, "Send Logic tick User->{}, server:{}", entity->ID(), entity->GetRecordServerId());

					entity->GetChannel()->deleteContextPtr();

					ServerEntityManagerHelper::Ptr serverEntityMan = dnServer->GetServerEntityManager();
					if(ServerEntityHelper::Ptr serverEntity = serverEntityMan->GetEntity(serverId))
					{
						request->set_accountid(entity->ID());

						MessagePackAndSend(0, EMMsgDeal::Redir, request, serverEntity->GetChannel());
					}

				}

			}

		}
		else
		{
			entity = entityMan->AddEntity(request->accountid());

			std::string token = Md5Hash(GetNowTimeStr());
			entity->SetToken(token);
			
			using namespace std::chrono;

			entity->SetExpireTime(duration_cast<seconds>(system_clock::now().time_since_epoch()).count() + 30);
		}

		response->set_token(entity->GetToken());
		response->set_expiredtimespan(entity->GetExpireTime());

		// entity or token expired
		if (!entity->GetTimerId())
		{
			entity->SetTimerId(entityMan->CheckEntityCloseTimer(entity->ID()));
		}

		LoggerPrint::Log(channel, ELogLevel_Debug, "ReqUserToken User: {}!!", request->accountid());
	};

	HandleRegistry<GMsg::A2g_ReqLogicServerIp, GMsg::g2A_ResLogicServerIp, EMMsgDeal::Req> Exe_ResLogicServerIp =
				[](auto request, auto response, const SocketChannel::Ptr& channel)
	{

		GateServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<GateServerHelper>(EMSystemType::Server);
		
		ServerEntityManagerHelper::Ptr serverEntityMan = dnServer->GetServerEntityManager();
		std::list<ServerEntity::Ptr> serverEntityList = serverEntityMan->GetEntitysByType(EMServerType::LogicServer);
		if(serverEntityList.size() > 0)
		{
			auto serverEntity = serverEntityList.front()->GetSelf<ServerEntityHelper>();
			response->set_serverip(serverEntity->GetServerIp());
			response->set_serverport(serverEntity->GetServerPort());
		}
		else
		{

		}
		
	};
}
