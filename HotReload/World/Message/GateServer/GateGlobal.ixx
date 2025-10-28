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

	HandleRegistry<GMsg::A2g_ReqAuthAccount, GMsg::g2A_ResAuthAccount, EMMsgDeal::Req> Exe_ReqUserToken(
				[](auto request, auto response, SocketChannel::Ptr channel)
	{

		GateServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<GateServerHelper>(EMSystemType::Server);
		ProxyEntityManagerHelper::Ptr entityMan = dnServer->GetProxyEntityManager();
		ProxyEntityHelper::Ptr entity = entityMan->GetEntity(request->accountid());
		if (entity)
		{
			//exit
			if (SocketChannel::Ptr online = entity->GetChannel())
			{
				// kick channel
				GMsg::S2C_RetAccountReplace notify_request;
				notify_request.set_serverip(request->serverip());
				
				std::string binData;

				notify_request.SerializeToString(&binData);
				MessagePackAndSend(0, EMMsgDeal::Ret, notify_request.GetDescriptor()->full_name(), binData, online);

				//kick socket
				online->deleteContextPtr();
				online->close();


				//kick game
				if (size_t serverId = entity->RecordServerId())
				{
					LoggerPrint::Log(channel, ELogLevel_Debug, "Send Logic tick User->{}, server:{}", entity->ID(), entity->RecordServerId());

					entity->GetChannel()->deleteContextPtr();

					ServerEntityManagerHelper::Ptr serverEntityMan = dnServer->GetServerEntityManager();
					if(ServerEntityHelper::Ptr serverEntity = serverEntityMan->GetEntity(serverId))
					{
						request->set_accountid(entity->ID());

						request->SerializeToString(&binData);
						MessagePackAndSend(0, EMMsgDeal::Redir, request->GetDescriptor()->full_name(), binData, serverEntity->GetChannel());
					}

				}

			}

		}
		else
		{
			entity = entityMan->AddEntity(request->accountid());

			std::string token = Md5Hash(GetNowTimeStr());
			entity->SetToken(token);
			

			entity->SetExpireTime(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count() + 30);
		}

		response->set_token(entity->Token());
		response->set_expiredtimespan(entity->ExpireTime());

		// entity or token expired
		if (!entity->TimerId())
		{
			entity->SetTimerId(entityMan->CheckEntityCloseTimer(entity->ID()));
		}

		LoggerPrint::Log(channel, ELogLevel_Debug, "ReqUserToken User: {}!!", request->accountid());
	});
}
