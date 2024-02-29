module;
#include "GlobalGate.pb.h"
#include "ProxyLogin.pb.h"
#include "hv/Channel.h"

#include <coroutine>
#include <string>
#include <chrono>
export module GateMessage:GateGlobal;

import MessagePack;
import GateServerHelper;
import Utils.StrUtils;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg::GlobalGate;
using namespace GMsg::ProxyLogin;

export void Exe_ReqUserToken(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	G2G_ReqUserToken* requset = (G2G_ReqUserToken*)msg;
	G2G_ResUserToken response;

	string binData;

	GateServerHelper* dnServer = GetGateServer();
	ProxyEntityHelper* entity = nullptr;
	if(entity = dnServer->GetProxyEntityManager()->GetEntity(requset->account_id()))
	{
		//exit
		if(SocketChannelPtr online = entity->GetChild()->GetSock())
		{
			// send to other client
			G2P_RetAccountReplace replace;
			replace.set_ip(requset->ip());

			binData.resize(replace.ByteSizeLong());
			replace.SerializeToArray(binData.data(), (int)binData.size());
			MessagePack(0, MsgDeal::Req, replace.GetDescriptor()->full_name().c_str(), binData);

			online->setContext(nullptr);
			online->write(binData);
			online->close();
			binData.clear();
		}

	}
	else
	{
		entity = dnServer->GetProxyEntityManager()->AddEntity(requset->account_id());

		string& token = entity->Token();
		token = GetNowTimeStr();
		token = Md5Hash(token);

		entity->ExpireTime() = chrono::duration_cast<chrono::seconds>(chrono::system_clock::now().time_since_epoch()).count();
		entity->ExpireTime() += 30;
	}
	
	
	response.set_token(entity->Token());
	response.set_expired_timespan(entity->ExpireTime());

	if(!entity->GetChild()->TimerId())
	{
		entity->GetChild()->TimerId() = dnServer->GetSSock()->loop(0)->setTimeout(30000, [entity](uint64_t timerId)
		{
			GetGateServer()->GetProxyEntityManager()->RemoveEntity(entity->GetChild()->ID());
		});
	}


	// server index 
	ServerEntity* serverEntity = dnServer->GetEntityManager()->GetEntityByList(ServerType::LogicServer).front();
	ServerEntityHelper* serverEntityHelper = static_cast<ServerEntityHelper*>(serverEntity);
	response.set_server_index(serverEntityHelper->GetChild()->ID());

	binData.resize(response.ByteSizeLong());
	response.SerializeToArray(binData.data(), (int)binData.size());
	MessagePack(msgId, MsgDeal::Res, response.GetDescriptor()->full_name().c_str(), binData);

	channel->write(binData);
}