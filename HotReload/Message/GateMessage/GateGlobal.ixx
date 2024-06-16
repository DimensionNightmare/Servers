module;
#include <coroutine>
#include <string>
#include <chrono>
#include <cstdint>

#include "StdMacro.h"
export module GateMessage:GateGlobal;

import MessagePack;
import GateServerHelper;
import StrUtils;
import Logger;
import Macro;
import ThirdParty.Libhv;
import ThirdParty.PbGen;

namespace GateMessage
{

	export void Exe_ReqUserToken(SocketChannelPtr channel, uint32_t msgId, Message* msg)
	{
		A2g_ReqAuthAccount* request = reinterpret_cast<A2g_ReqAuthAccount*>(msg);
		g2A_ResAuthAccount response;

		string binData;
		string* reqIp = request->mutable_ip();

		GateServerHelper* dnServer = GetGateServer();
		ProxyEntityManagerHelper* entityMan = dnServer->GetProxyEntityManager();
		ProxyEntity* entity = entityMan->GetEntity(request->account_id());
		if (entity)
		{
			//exit
			if (const SocketChannelPtr& online = entity->GetSock())
			{
				// kick channel
				S2C_RetAccountReplace request;
				request.set_ip(*reqIp);

				request.SerializeToString(&binData);
				MessagePack(0, MsgDeal::Ret, request.GetDescriptor()->full_name().c_str(), binData);

				//kick socket
				online->write(binData);
				online->setContext(nullptr);
				online->close();


				//kick game
				if (uint32_t serverIndex = entity->ServerIndex())
				{
					DNPrint(0, LoggerLevel::Debug, "Send Logic tick User->%d, server:%d", entity->ID(), entity->ServerIndex());

					ServerEntityManagerHelper* serverEntityMan = dnServer->GetServerEntityManager();
					ServerEntity* serverEntity = serverEntityMan->GetEntity(serverIndex);

					request.set_account_id(entity->ID());

					request.SerializeToString(&binData);
					MessagePack(0, MsgDeal::Redir, request.GetDescriptor()->full_name().c_str(), binData);
					serverEntity->GetSock()->write(binData);
				}

			}

		}
		else
		{
			entity = entityMan->AddEntity(request->account_id());

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

		DNPrint(0, LoggerLevel::Debug, "ReqUserToken User: %d!!", request->account_id());

		response.SerializeToString(&binData);
		MessagePack(msgId, MsgDeal::Res, nullptr, binData);

		channel->write(binData);
	}

}
