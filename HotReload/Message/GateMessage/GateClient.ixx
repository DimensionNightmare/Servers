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
import DNServerProxyHelper;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg::C_Auth;
using namespace GMsg::S_Logic;

// client request
export DNTaskVoid Msg_ReqAuthToken(const SocketChannelPtr &channel, uint32_t msgId, Message *msg)
{
	C2S_ReqAuthToken* requset = reinterpret_cast<C2S_ReqAuthToken*>(msg);

	GateServerHelper* dnServer = GetGateServer();
	ProxyEntityManagerHelper<ProxyEntity>* entityMan = dnServer->GetProxyEntityManager();

	S2C_ResAuthToken response;
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
			DNPrint(0, LoggerLevel::Debug, "match!!\n");
			if(uint64_t timerId = entity->TimerId())
			{
				entity->TimerId() = 0;
				entityMan->Timer()->killTimer(timerId);

				entity->CloseEvent() = std::bind(&GateServerHelper::ProxyEntityCloseEvent, dnServer, std::placeholders::_1);
			}


			ServerEntityManagerHelper<ServerEntity>* serverEntityMan = dnServer->GetServerEntityManager();
			list<ServerEntity*> serverEntityList = serverEntityMan->GetEntityByList(ServerType::LogicServer);
			ServerEntity* serverEntity = nullptr;
			if(!serverEntityList.empty())
			{
				serverEntity = serverEntityList.front();
			}

			if(serverEntity)
			{
				G2L_ReqClientLogin requestChild;
				requestChild.set_account_id(entity->ID());
				binData.resize(requestChild.ByteSize());
				requestChild.SerializeToArray(binData.data(), binData.size());

				DNServerProxyHelper* server = dnServer->GetSSock();
				uint32_t msgIdChild = server->GetMsgId();

				MessagePack(msgIdChild, MsgDeal::Req, requestChild.GetDescriptor()->full_name().c_str(), binData);

				L2G_ResClientLogin responseChild;

				auto dataChannel = [&responseChild]()->DNTask<Message>
				{
					co_return responseChild;
				}();

				server->AddMsg(msgIdChild, &dataChannel);

				ServerEntityHelper* serverEntityHelper = static_cast<ServerEntityHelper*>(serverEntity);
				serverEntityHelper->GetSock()->write(binData);
				binData.clear();

				co_await dataChannel;

				if(responseChild.has_ds_info())
				{
					response.mutable_ds_info()->MergeFrom(responseChild.ds_info());
					DNPrint(0, LoggerLevel::Debug, "has ds:%s", response.mutable_ds_info()->ip().c_str());
				}
			}
			else
			{
				DNPrint(0, LoggerLevel::Debug, "Msg_ReqAuthToken not LogicServer !!");
			}
			
	
		}
		else
		{
			DNPrint(0, LoggerLevel::Debug, "not match!!\n");
		}
	}
	else
	{
		DNPrint(0, LoggerLevel::Debug, "noaccount !!\n");
	}

	
	binData.resize(response.ByteSize());
	response.SerializeToArray(binData.data(), binData.size());

	MessagePack(msgId, MsgDeal::Res, nullptr, binData);
	
	channel->write(binData);

	co_return;
}