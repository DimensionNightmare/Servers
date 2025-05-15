module;
export module GateMessage:GateGlobal;

import FuncHelper;
import GateServerHelper;
import StrUtils;
import Logger;
import DllUtils;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import ProxyEntityManagerHelper;
import std.compat;

#define FUNCPLACE(func) #func, func

namespace GateMessage
{

	export void Exe_ReqUserToken(hv::SocketChannelPtr channel, uint32_t msgId, std::string binMsg)
	{
		GMsg::A2g_ReqAuthAccount request;
		
		if(!request.ParseFromString(binMsg))
		{
			return;
		}
		
		GMsg::g2A_ResAuthAccount response;

		std::string binData;

		GateServerHelper* dnServer = GetGateServer();
		ProxyEntityManagerHelper* entityMan = dnServer->GetProxyEntityManager();
		ProxyEntity::Ptr entity = entityMan->GetEntity(request.account_id());
		if (entity)
		{
			//exit
			if (const hv::SocketChannelPtr& online = entity->GetSock())
			{
				// kick channel
				GMsg::S2C_RetAccountReplace request;
				request.set_server_ip(request.server_ip());

				request.SerializeToString(&binData);
				MessagePackAndSend(0, EMMsgDeal::Ret, request.GetDescriptor()->full_name(), binData, online);

				//kick socket
				online->setContext(nullptr);
				online->close();


				//kick game
				if (uint32_t serverId = entity->RecordServerId())
				{
					SPidLogger.Record(ELogLevel_Debug, "Send Logic tick User->{}, server:{}", entity->ID(), entity->RecordServerId());

					ServerEntityManagerHelper* serverEntityMan = dnServer->GetServerEntityManager();
					ServerEntity::Ptr serverEntity = serverEntityMan->GetEntity(serverId);

					request.set_account_id(entity->ID());

					request.SerializeToString(&binData);
					MessagePackAndSend(0, EMMsgDeal::Redir, request.GetDescriptor()->full_name(), binData, serverEntity->GetSock());
				}

			}

		}
		else
		{
			entity = entityMan->AddEntity(request.account_id());

			std::string& token = entity->Token();
			token = GetNowTimeStr();
			token = Md5Hash(token);

			entity->ExpireTime() = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			entity->ExpireTime() += 30;
		}

		response.set_token(entity->Token());
		response.set_expired_timespan(entity->ExpireTime());

		// entity or token expired
		if (!entity->TimerId())
		{
			entity->TimerId() = TickMainSpaceDll(entityMan, FUNCPLACE(&ProxyEntityManager::CheckEntityCloseTimer), entity->ID());
		}

		SPidLogger.Record(ELogLevel_Debug, "ReqUserToken User: {}!!", request.account_id());

		response.SerializeToString(&binData);
		MessagePackAndSend(msgId, EMMsgDeal::Res, "", binData, channel);
	}

}
