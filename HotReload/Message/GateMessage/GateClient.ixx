module;
#include <coroutine>
#include <cstdint>
#include <list>
#include "hv/Channel.h"

#include "StdAfx.h"
#include "Client/C_Auth.pb.h"
#include "Server/S_Gate_Logic.pb.h" 
export module GateMessage:GateClient;

import GateServerHelper;
import DNTask;
import Utils.StrUtils;
import MessagePack;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg;

// client request
export DNTaskVoid Msg_ReqAuthToken(const SocketChannelPtr &channel, uint32_t msgId, Message *msg)
{
	C2S_ReqAuthToken* requset = reinterpret_cast<C2S_ReqAuthToken*>(msg);

	GateServerHelper* dnServer = GetGateServer();
	ProxyEntityManagerHelper* entityMan = dnServer->GetProxyEntityManager();

	S2C_ResAuthToken response;
	string binData;

	if(ProxyEntityHelper* entity = entityMan->GetEntity(requset->account_id()))
	{
		string md5 = Md5Hash(entity->Token());
		bool isMatch = md5 == requset->token();
		
		// if not match, timer will destory entity
		if(isMatch)
		{
			channel->setContext(entity);
			entity->SetSock(channel);
			DNPrint(0, LoggerLevel::Debug, "match!!\n");
			if(uint64_t timerId = entity->TimerId())
			{
				entity->TimerId() = 0;
				entityMan->Timer()->killTimer(timerId);
			}
			
			ServerEntityManagerHelper* serverEntityMan = dnServer->GetServerEntityManager();
			ServerEntityHelper* serverEntity = nullptr;

			// <cache> server to load login data
			if(uint32_t serverIndex = entity->ServerIndex())
			{
				serverEntity = serverEntityMan->GetEntity(serverIndex);
			}
			else
			{
				list<ServerEntity*> serverEntityList = serverEntityMan->GetEntityByList(ServerType::LogicServer);
				if(!serverEntityList.empty())
				{
					serverEntity = static_cast<ServerEntityHelper*>(serverEntityList.front());
				}
			}
			
			//req dedicatedServer Info to Login ds.
			if(serverEntity)
			{

				entity->ServerIndex() = serverEntity->ID();
			
				//redirect G2L_ReqClientLogin dot pack string
				requset->clear_token();
				binData.clear();
				binData.resize(requset->ByteSizeLong());
				requset->SerializeToArray(binData.data(), binData.size());

				DNServerProxyHelper* server = dnServer->GetSSock();
				uint32_t msgIdChild = server->GetMsgId();

				MessagePack(msgIdChild, MsgDeal::Req, G2L_ReqClientLogin::GetDescriptor()->full_name().c_str(), binData);

				auto dataChannel = [&response]()->DNTask<Message>
				{
					co_return response;
				}();

				{
					ServerEntityHelper* serverEntityHelper = static_cast<ServerEntityHelper*>(serverEntity);
					server->AddMsg(msgIdChild, &dataChannel, 9000);
					serverEntityHelper->GetSock()->write(binData);
					co_await dataChannel;
					if(dataChannel.HasFlag(DNTaskFlag::Timeout))
					{
						DNPrint(0, LoggerLevel::Debug, "requst timeout! \n");
					}

				}

				if(!response.state_code())
				{
					DNPrint(0, LoggerLevel::Debug, "has ds:%s", response.ip().c_str());
				}

			}
			else
			{
				response.set_state_code(3);
				DNPrint(0, LoggerLevel::Debug, "Msg_ReqAuthToken not LogicServer !!");
			}
			
	
		}
		else
		{
			response.set_state_code(1);
			DNPrint(0, LoggerLevel::Debug, "not match!!\n");
		}
	}
	else
	{
		response.set_state_code(2);
		DNPrint(0, LoggerLevel::Debug, "noaccount !!\n");
	}

	binData.clear();
	binData.resize(response.ByteSizeLong());
	response.SerializeToArray(binData.data(), binData.size());

	MessagePack(msgId, MsgDeal::Res, nullptr, binData);
	
	channel->write(binData);

	co_return;
}