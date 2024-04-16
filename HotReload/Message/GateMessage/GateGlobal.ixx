module;
#include "S_Global.pb.h"
#include "C_Auth.pb.h"
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
using namespace GMsg::S_Global;
using namespace GMsg::C_Auth;

export void Exe_ReqUserToken(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	G2G_ReqLoginToken* requset = reinterpret_cast<G2G_ReqLoginToken*>(msg);
	G2G_ResLoginToken response;

	string binData;

	GateServerHelper* dnServer = GetGateServer();
	ProxyEntityManagerHelper<ProxyEntity>* entityMan = dnServer->GetProxyEntityManager();
	ProxyEntityHelper* entity = nullptr;
	if(entity = entityMan->GetEntity(requset->account_id()))
	{
		//exit
		if(SocketChannelPtr online = entity->GetChild()->GetSock())
		{
			// send to other client
			S2C_RetAccountReplace replace;
			replace.set_ip(requset->ip());

			binData.resize(replace.ByteSizeLong());
			replace.SerializeToArray(binData.data(), (int)binData.size());
			MessagePack(0, MsgDeal::Ret, replace.GetDescriptor()->full_name().c_str(), binData);

			online->setContext(nullptr);
			online->write(binData);
			online->close();
			binData.clear();
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
	if(!entity->GetChild()->TimerId())
	{
		entity->GetChild()->TimerId() = entityMan->Timer()->setTimeout(30000, 
			std::bind(&ProxyEntityManager<ProxyEntity>::EntityCloseTimer, entityMan, placeholders::_1));

		entityMan->AddTimerRecord(entity->GetChild()->TimerId(), entity->GetChild()->ID());
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