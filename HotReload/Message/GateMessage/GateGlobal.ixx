module;
#include <coroutine>
#include <string>
#include <chrono>
#include <cstdint>
#include "hv/Channel.h"

#include "StdMacro.h"
#include "Server/S_Global_Gate.pb.h"
#include "Client/C_Auth.pb.h"
#include "Server/S_Gate_Logic.pb.h"
export module GateMessage:GateGlobal;

import MessagePack;
import GateServerHelper;
import Utils.StrUtils;
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
		G2g_ReqLoginToken* requset = reinterpret_cast<G2g_ReqLoginToken*>(msg);
		g2G_ResLoginToken response;

		string binData;

		GateServerHelper* dnServer = GetGateServer();
		ProxyEntityManagerHelper* entityMan = dnServer->GetProxyEntityManager();
		ProxyEntityHelper* entity = entityMan->GetEntity(requset->account_id());
		if (entity)
		{
			//exit
			if (const SocketChannelPtr& online = entity->GetSock())
			{
				// kick channel
				S2C_RetAccountReplace retMsg;
				retMsg.set_ip(requset->ip());

				binData.clear();
				binData.resize(retMsg.ByteSizeLong());
				retMsg.SerializeToArray(binData.data(), binData.size());
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
					ServerEntityHelper* serverEntity = serverEntityMan->GetEntity(serverIndex);

					G2L_RetAccountReplace retMsg;
					retMsg.set_account_id(entity->ID());
					retMsg.set_ip(requset->ip());

					binData.clear();
					binData.resize(retMsg.ByteSizeLong());
					retMsg.SerializeToArray(binData.data(), binData.size());
					MessagePack(0, MsgDeal::Ret, retMsg.GetDescriptor()->full_name().c_str(), binData);
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

		binData.clear();
		binData.resize(response.ByteSizeLong());
		response.SerializeToArray(binData.data(), binData.size());
		MessagePack(msgId, MsgDeal::Res, nullptr, binData);

		channel->write(binData);
	}
}
