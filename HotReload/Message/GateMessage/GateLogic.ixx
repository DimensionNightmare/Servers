module;
#include "StdAfx.h"
#include "C_Auth.pb.h"
#include "hv/Channel.h"

#include <coroutine>
export module GateMessage:GateLogic;

import GateServerHelper;
import DNTask;
import Utils.StrUtils;
import MessagePack;

import Entity;
import ProxyEntityHelper;

using namespace std;
using namespace GMsg::C_Auth;
using namespace google::protobuf;
using namespace hv;

void ProxyEntityCloseEvent(Entity* entity)
{
	ProxyEntityHelper* castObj = static_cast<ProxyEntityHelper*>(entity);

	GateServerHelper* dnServer = GetGateServer();

	auto entityMan = dnServer->GetProxyEntityManager();
	entityMan->RemoveEntity(castObj->GetChild()->ID());
}

// client request
export void Msg_ReqAuthToken(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	C2S_ReqAuthToken* requset = reinterpret_cast<C2S_ReqAuthToken*>(msg);

	GateServerHelper* dnServer = GetGateServer();
	ProxyEntityManagerHelper<ProxyEntity>* entityMan = dnServer->GetProxyEntityManager();

	C2S_ResAuthToken response;

	if(ProxyEntityHelper* entity = entityMan->GetEntity(requset->account_id()))
	{
		string md5 = Md5Hash(entity->Token());
		bool isMatch = md5 == requset->token();
		response.set_success(isMatch);

		if(isMatch)
		{
			channel->setContext(entity);
			entity->GetChild()->SetSock(channel);
			DNPrint(-1, LoggerLevel::Debug, "match!!\n");
			if(uint64_t timerId = entity->GetChild()->TimerId())
			{
				entity->GetChild()->TimerId() = 0;
				entityMan->Timer()->killTimer(timerId);

				entity->CloseEvent() = &ProxyEntityCloseEvent;
			}
		}
		else
		{
			DNPrint(-1, LoggerLevel::Debug, "not match!!\n");
		}
	}
	else
	{
		response.set_success(false);
		DNPrint(-1, LoggerLevel::Debug, "noaccount !!\n");
	}

	string binData;
	binData.resize(response.ByteSize());
	response.SerializeToArray(binData.data(), binData.size());

	MessagePack(msgId, MsgDeal::Res, nullptr, binData);
	
	channel->write(binData);
}