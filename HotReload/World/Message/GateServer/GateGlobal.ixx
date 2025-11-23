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
				[](World::Ptr world, SocketChannel::Ptr channel, auto request, auto response)
	{
		
		GateServerHelper::CVPtr dnServer = world->GetSystem<GateServerHelper>(EMSystemType::Server);
		ProxyEntityManagerHelper::CVPtr entityMan = dnServer->GetProxyEntityManager();
		ProxyEntityHelper::Ptr entity = entityMan->GetEntity(request->accountid());
		if (entity)
		{
			//exit
			if (SocketChannel::CVPtr online = entity->GetChannel())
			{
				// kick channel
				GMsg::S2C_RetAccountReplace notify_request;
				notify_request.set_serverip(request->serverip());

				ServerProxyHelper::CVPtr proxyHelper = dnServer->GetServerProxy();
				
				proxyHelper->AddMsg(EMMsgDeal::Ret, &notify_request, online).Resume();

				//kick socket
				online->close();


				//kick game
				if (size_t serverId = entity->GetRecordServerId())
				{
					LoggerPrint::Log(world, ELogLevel_Debug, "Send Logic tick User->{}, server:{}", entity->ID(), entity->GetRecordServerId());

					ServerEntityManagerHelper::CVPtr serverEntityMan = dnServer->GetServerEntityManager();
					if(ServerEntityHelper::CVPtr serverEntity = serverEntityMan->GetEntity(serverId))
					{
						request->set_accountid(entity->ID());

						ServerProxyHelper::CVPtr proxyHelper = dnServer->GetServerProxy();
				
						proxyHelper->AddMsg(EMMsgDeal::Redir, request, serverEntity->GetChannel()).Resume();
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

		LoggerPrint::Log(world, ELogLevel_Debug, "ReqUserToken User: {}!!", request->accountid());
	};

	HandleRegistry<GMsg::A2g_ReqLogicServerIp, GMsg::g2A_ResLogicServerIp, EMMsgDeal::Req> Exe_ResLogicServerIp =
				[](World::Ptr world, SocketChannel::Ptr channel, auto request, auto response)
	{
		
		GateServerHelper::CVPtr dnServer = world->GetSystem<GateServerHelper>(EMSystemType::Server);

		auto selects = dnServer->GetServerEntityManager()->GetEntitysByType(EMServerType::LogicServer)
			| std::views::transform([](const auto& param){
				return param->GetSelf<ServerEntityHelper>();
			})
			| std::views::filter([](const auto& param)
				{
					return param->GetTimerId() == 0;
				})
			;

		if(auto it = std::ranges::min_element(selects, std::greater{}, &ServerEntityHelper::GetConnNum); it != selects.end())
		{
			ServerEntityHelper::CVPtr serverEntity = *it;
			response->set_serverip(serverEntity->GetServerIp());
			response->set_serverport(serverEntity->GetServerPort());
		}
		else
		{

		}
		
	};
}
