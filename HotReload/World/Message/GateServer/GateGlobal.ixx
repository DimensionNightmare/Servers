export module GateServerMessage:GateGlobal;

import GateServerHelper;
import ProxyEntityHelper;
import StrUtils;
import ThirdParty.Libhv;
import FuncHelper;
import std;
import ThirdParty.PbGen;
import Logger;

export namespace GateServerMessage
{

	void Exe_ReqUserToken(SocketChannel::CVPtr channel, uint32_t msgId, const std::string& binMsg)
	{
		GMsg::A2g_ReqAuthAccount request;
		
		if(!request.ParseFromString(binMsg))
		{
			return;
		}
		
		GMsg::g2A_ResAuthAccount response;

		std::string binData;

		GateServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<GateServerHelper>(EMSystemType::Server);
		ProxyEntityManagerHelper::Ptr entityMan = dnServer->GetProxyEntityManager();
		ProxyEntityHelper::Ptr entity = entityMan->GetEntity(request.accountid());
		if (entity)
		{
			//exit
			if (SocketChannel::CVPtr online = entity->GetChannel())
			{
				// kick channel
				GMsg::S2C_RetAccountReplace notify_request;
				notify_request.set_serverip(request.serverip());

				notify_request.SerializeToString(&binData);
				MessagePackAndSend(0, EMMsgDeal::Ret, notify_request.GetDescriptor()->full_name(), binData, online);

				//kick socket
				online->deleteContextPtr();
				online->close();


				//kick game
				if (uint64_t serverId = entity->RecordServerId())
				{
					LoggerPrint::Log(channel, ELogLevel_Debug, "Send Logic tick User->{}, server:{}", entity->ID(), entity->RecordServerId());

					entity->GetChannel()->deleteContextPtr();

					ServerEntityManagerHelper::Ptr serverEntityMan = dnServer->GetServerEntityManager();
					if(ServerEntityHelper::CVPtr serverEntity = serverEntityMan->GetEntity(serverId))
					{
						request.set_accountid(entity->ID());

						request.SerializeToString(&binData);
						MessagePackAndSend(0, EMMsgDeal::Redir, request.GetDescriptor()->full_name(), binData, serverEntity->GetChannel());
					}

				}

			}

		}
		else
		{
			entity = entityMan->AddEntity(request.accountid());

			std::string token = Md5Hash(GetNowTimeStr());
			entity->SetToken(token);
			

			entity->SetExpireTime(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count() + 30);
		}

		response.set_token(entity->Token());
		response.set_expiredtimespan(entity->ExpireTime());

		// entity or token expired
		if (!entity->TimerId())
		{
			entity->SetTimerId(entityMan->CheckEntityCloseTimer(entity->ID()));
		}

		LoggerPrint::Log(channel, ELogLevel_Debug, "ReqUserToken User: {}!!", request.accountid());

		response.SerializeToString(&binData);
		MessagePackAndSend(msgId, EMMsgDeal::Res, binData, channel);
	}

}
