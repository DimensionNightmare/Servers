module;
#include "StdMacro.h"
export module GateMessage:GateGlobal;

import FuncHelper;
import GateServerHelper;
import StrUtils;
import Logger;
import Macro;
import ThirdParty.Libhv;
import ThirdParty.PbGen;

namespace GateMessage
{

	export void Exe_ReqUserToken(SocketChannelPtr channel, uint32_t msgId,  string binMsg)
	{
		A2g_ReqAuthAccount request;
		
		if(!request.ParseFromString(binMsg))
		{
			return;
		}
		
		g2A_ResAuthAccount response;

		string binData;

		GateServerHelper* dnServer = GetGateServer();
		ProxyEntityManagerHelper* entityMan = dnServer->GetProxyEntityManager();
		ProxyEntity* entity = entityMan->GetEntity(request.account_id());
		if (entity)
		{
			//exit
			if (const SocketChannelPtr& online = entity->GetSock())
			{
				// kick channel
				S2C_RetAccountReplace request;
				request.set_server_ip(request.server_ip());

				request.SerializeToString(&binData);
				MessagePackAndSend(0, MsgDeal::Ret, request.GetDescriptor()->full_name().c_str(), binData, online);

				//kick socket
				online->setContext(nullptr);
				online->close();


				//kick game
				if (uint32_t serverId = entity->RecordServerId())
				{
					DNPrint(0, LoggerLevel::Debug, "Send Logic tick User->%d, server:%d", entity->ID(), entity->RecordServerId());

					ServerEntityManagerHelper* serverEntityMan = dnServer->GetServerEntityManager();
					ServerEntity* serverEntity = serverEntityMan->GetEntity(serverId);

					request.set_account_id(entity->ID());

					request.SerializeToString(&binData);
					MessagePackAndSend(0, MsgDeal::Redir, request.GetDescriptor()->full_name().c_str(), binData, serverEntity->GetSock());
				}

			}

		}
		else
		{
			entity = entityMan->AddEntity(request.account_id());

			string& token = entity->Token();
			token = GetNowTimeStr();
			token = Md5Hash(token);

			entity->ExpireTime() = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).count();
			entity->ExpireTime() += 30;
		}

		response.set_token(entity->Token());
		response.set_expired_timespan(entity->ExpireTime());

		// entity or token expired
		if (!entity->TimerId())
		{
			entity->TimerId() = TICK_MAINSPACE_SIGN_FUNCTION(ProxyEntityManager, CheckEntityCloseTimer, entityMan, entity->ID());
		}

		DNPrint(0, LoggerLevel::Debug, "ReqUserToken User: %d!!", request.account_id());

		response.SerializeToString(&binData);
		MessagePackAndSend(msgId, MsgDeal::Res, nullptr, binData, channel);
	}

}
