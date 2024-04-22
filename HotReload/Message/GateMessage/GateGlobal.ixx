module;
#include <coroutine>
#include <string>
#include <chrono>
#include "hv/Channel.h"

#include "StdAfx.h"
#include "Server/S_Global.pb.h"
#include "Client/C_Auth.pb.h"
export module GateMessage:GateGlobal;

import MessagePack;
import GateServerHelper;
import Utils.StrUtils;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg::S_Global;
using namespace GMsg::C_Auth;

export void Exe_ReqUserToken(const SocketChannelPtr &channel, uint32_t msgId, Message *msg)
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
		if(SocketChannelPtr online = entity->GetSock())
		{
			// send to other client
			S2C_RetAccountReplace retMsg;
			retMsg.set_ip(requset->ip());

			binData.resize(retMsg.ByteSizeLong());
			retMsg.SerializeToArray(binData.data(), (int)binData.size());
			MessagePack(0, MsgDeal::Ret, retMsg.GetDescriptor()->full_name().c_str(), binData);

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
	if(!entity->TimerId())
	{
		entity->TimerId() = entityMan->Timer()->setTimeout(30000, 
			std::bind(&ProxyEntityManager<ProxyEntity>::EntityCloseTimer, entityMan, placeholders::_1));

		entityMan->AddTimerRecord(entity->TimerId(), entity->ID());
	}


	// server index 
	ServerEntityManagerHelper<ServerEntity>* serverEntityMan = dnServer->GetServerEntityManager();
	list<ServerEntity*> serverEntityList = serverEntityMan->GetEntityByList(ServerType::LogicServer);
	ServerEntity* serverEntity = nullptr;
	if(!serverEntityList.empty())
	{
		serverEntity = serverEntityList.front();
	}

	if(serverEntity)
	{
		ServerEntityHelper* serverEntityHelper = static_cast<ServerEntityHelper*>(serverEntity);
		response.set_server_index(serverEntityHelper->ID());

		binData.resize(response.ByteSizeLong());
		response.SerializeToArray(binData.data(), (int)binData.size());
		MessagePack(msgId, MsgDeal::Res, response.GetDescriptor()->full_name().c_str(), binData);

		channel->write(binData);
	}
	else
	{
		DNPrint(0, LoggerLevel::Debug, "Exe_ReqUserToken not LogicServer !!");
	}
	
}