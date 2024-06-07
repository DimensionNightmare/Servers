module;
#include <coroutine>
#include <string>
#include <chrono>
#include <cstdint>
#include "hv/Channel.h"

#include "StdMacro.h"
#include "Server/S_Global.pb.h"
#include "Client/C_Auth.pb.h"
#include "Server/S_Gate.pb.h"
#include "Server/S_Auth.pb.h"
export module GateMessage:GateRedirect;

import MessagePack;
import GateServerHelper;
import StrUtils;
import Logger;
import Macro;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg;

namespace GateMessage
{

	export void Exe_ReqUserToken(SocketChannelPtr channel, uint32_t msgId, Message* msg)
	{
		A2g_ReqAuthAccount* requset = reinterpret_cast<A2g_ReqAuthAccount*>(msg);
		g2A_ResAuthAccount response;

		string binData;

		GateServerHelper* dnServer = GetGateServer();
		ProxyEntityManagerHelper* entityMan = dnServer->GetProxyEntityManager();
		ProxyEntity* entity = entityMan->GetEntity(requset->account_id());
		if (entity)
		{
			//exit
			if (const SocketChannelPtr& online = entity->GetSock())
			{
				// kick channel
				S2C_RetAccountReplace retMsg;
				retMsg.set_ip(requset->ip());

				retMsg.SerializeToString(&binData);
				MessagePack(0, MsgDeal::Ret, retMsg.GetDescriptor()->full_name().c_str(), binData);

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

					retMsg.set_account_id(entity->ID());

					retMsg.SerializeToString(&binData);
					MessagePack(0, MsgDeal::Redir, retMsg.GetDescriptor()->full_name().c_str(), binData);
					serverEntity->GetSock()->write(binData);
				}

			}

		}
		else
		{
			entity = entityMan->AddEntity(requset->account_id());

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

		DNPrint(0, LoggerLevel::Debug, "ReqUserToken User: %d!!", requset->account_id());

		response.SerializeToString(&binData);
		MessagePack(msgId, MsgDeal::Res, nullptr, binData);

		channel->write(binData);
	}
}
