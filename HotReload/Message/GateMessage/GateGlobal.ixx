module;
export module GateMessage:GateGlobal;

import FuncHelper;
import StrUtils;
import Logger;
import DllUtils;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import ProxyEntityManagerHelper;
import std.compat;
import DNSocketProxy;
import ServerEntity;
import GateServerHelper;
import ECSW;

#define FUNCPLACE(func) #func, func

namespace GateMessage
{

	export void Exe_ReqUserToken(const World::Ptr& world, const DNSocketProxy::Ptr& channel, uint32_t msgId, std::string binMsg)
	{
		GMsg::A2g_ReqAuthAccount request;
		
		if(!request.ParseFromString(binMsg))
		{
			return;
		}
		
		GMsg::g2A_ResAuthAccount response;

		std::string binData;

		GateServerHelper::Ptr dnServer = world->GetSystem<GateServerHelper>(EMSystemType::DNServer);
		ProxyEntityManagerHelper::Ptr entityMan = dnServer->GetProxyEntityManager();
		ProxyEntity::Ptr entity = entityMan->GetEntity(request.account_id());
		if (entity)
		{
			//exit
			if (const DNSocketProxy::Ptr& online = entity->GetSock())
			{
				// kick channel
				GMsg::S2C_RetAccountReplace request;
				request.set_server_ip(request.server_ip());

				request.SerializeToString(&binData);
				MessagePackAndSend(0, EMMsgDeal::Ret, request.GetDescriptor()->full_name(), binData, online);

				//kick socket
				online->setContextPtr(nullptr);
				online->close();


				//kick game
				if (uint64_t serverId = entity->RecordServerId())
				{
					SPidLogger.Record(ELogLevel_Debug, "Send Logic tick User->{}, server:{}", entity->ID(), entity->RecordServerId());

					ServerEntityManagerHelper::Ptr serverEntityMan = dnServer->GetServerEntityManager();
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

			std::string token = Md5Hash(GetNowTimeStr());
			entity->SetToken(token);
			

			entity->SetExpireTime(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count() + 30);
		}

		response.set_token(entity->Token());
		response.set_expired_timespan(entity->ExpireTime());

		// entity or token expired
		if (!entity->TimerId())
		{
			entity->SetTimerId(TickMainSpaceDll(entityMan.get(), FUNCPLACE(&ProxyEntityManager::CheckEntityCloseTimer), entity->ID()));
		}

		SPidLogger.Record(ELogLevel_Debug, "ReqUserToken User: {}!!", request.account_id());

		response.SerializeToString(&binData);
		MessagePackAndSend(msgId, EMMsgDeal::Res, "", binData, channel);
	}

}
