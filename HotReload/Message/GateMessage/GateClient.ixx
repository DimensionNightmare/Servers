module;
#include <coroutine>
#include "hv/Channel.h"

#include "StdAfx.h"
#include "Client/C_Auth.pb.h"
#include "Server/S_Logic.pb.h" 
export module GateMessage:GateClient;

import GateServerHelper;
import DNTask;
import Utils.StrUtils;
import MessagePack;
import ProxyEntityHelper;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg::C_Auth;
using namespace GMsg::S_Logic;

// client request
export void Msg_ReqAuthToken(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	C2S_ReqAuthToken* requset = reinterpret_cast<C2S_ReqAuthToken*>(msg);

	GateServerHelper* dnServer = GetGateServer();
	ProxyEntityManagerHelper<ProxyEntity>* entityMan = dnServer->GetProxyEntityManager();

	C2S_ResAuthToken response;
	string binData;

	if(ProxyEntityHelper* entity = entityMan->GetEntity(requset->account_id()))
	{
		string md5 = Md5Hash(entity->Token());
		bool isMatch = md5 == requset->token();
		response.set_success(isMatch);

		if(isMatch)
		{
			channel->setContext(entity);
			entity->SetSock(channel);
			DNPrint(-1, LoggerLevel::Debug, "match!!\n");
			if(uint64_t timerId = entity->TimerId())
			{
				entity->TimerId() = 0;
				entityMan->Timer()->killTimer(timerId);

				entity->CloseEvent() = std::bind(&GateServerHelper::ProxyEntityCloseEvent, dnServer, std::placeholders::_1);
			}

			G2L_RetClientLogin retMsg;
			retMsg.set_account_id(entity->ID());
			binData.resize(response.ByteSize());
			response.SerializeToArray(binData.data(), binData.size());

			MessagePack(0, MsgDeal::Ret, retMsg.GetDescriptor()->full_name().c_str(), binData);

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
				serverEntityHelper->GetSock()->write(binData);
			}
			else
			{
				DNPrint(-1, LoggerLevel::Error, "Msg_ReqAuthToken not LogicServer !!");
			}
			
	
			binData.clear();
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

	
	binData.resize(response.ByteSize());
	response.SerializeToArray(binData.data(), binData.size());

	MessagePack(msgId, MsgDeal::Res, nullptr, binData);
	
	channel->write(binData);
}