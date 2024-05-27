module;
#include <coroutine>
#include <cstdint>
#include <list>
#include "hv/Channel.h"

#include "StdMacro.h"
#include "Client/C_Auth.pb.h"
#include "Server/S_Gate_Logic.pb.h" 
export module GateMessage:GateClient;

import GateServerHelper;
import DNTask;
import Utils.StrUtils;
import MessagePack;
import Logger;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg;

namespace GateMessage
{

	// client request
	export DNTaskVoid Msg_ReqAuthToken(SocketChannelPtr channel, uint32_t msgId, Message* msg)
	{
		C2S_ReqAuthToken* requset = reinterpret_cast<C2S_ReqAuthToken*>(msg);

		GateServerHelper* dnServer = GetGateServer();
		ProxyEntityManagerHelper* entityMan = dnServer->GetProxyEntityManager();

		S2C_ResAuthToken response;
		string binData;

		ProxyEntityHelper* entity = entityMan->GetEntity(requset->account_id());
		if (!entity)
		{
			DNPrint(0, LoggerLevel::Debug, "noaccount !!");
			response.set_state_code(1);
		}
		// if not match, timer will destory entity
		else if (Md5Hash(entity->Token()) != requset->token())
		{
			DNPrint(0, LoggerLevel::Debug, "not match!!");
			response.set_state_code(2);
		}
		else
		{
			DNPrint(0, LoggerLevel::Debug, "match!!");

			channel->setContext(entity);
			entity->SetSock(channel);

			if (uint64_t timerId = entity->TimerId())
			{
				entity->TimerId() = 0;
				entityMan->Timer()->killTimer(timerId);
			}

			//DS Server
			ServerEntityManagerHelper* serverEntityMan = dnServer->GetServerEntityManager();
			ServerEntityHelper* serverEntity = nullptr;

			// <cache> server to load login data
			if (uint32_t serverIndex = entity->ServerIndex())
			{
				serverEntity = serverEntityMan->GetEntity(serverIndex);
			}

			// pool
			if (!serverEntity)
			{
				list<ServerEntity*> serverEntityList = serverEntityMan->GetEntityByList(ServerType::LogicServer);
				if (serverEntityList.empty())
				{
					DNPrint(0, LoggerLevel::Debug, "Msg_ReqAuthToken not LogicServer !!");
					response.set_state_code(3);
				}
				else
				{
					serverEntity = static_cast<ServerEntityHelper*>(serverEntityList.front());
				}
			}

			//req dedicatedServer Info to Login ds.
			if (serverEntity)
			{
				entity->ServerIndex() = serverEntity->ID();

				DNPrint(0, LoggerLevel::Debug, "Send to Logic index->%d, %d", entity->ID(), entity->ServerIndex());

				//redirect G2L_ReqClientLogin dot pack string
				requset->clear_token();
				binData.clear();
				binData.resize(requset->ByteSizeLong());
				requset->SerializeToArray(binData.data(), binData.size());

				DNServerProxyHelper* server = dnServer->GetSSock();
				uint32_t msgIdChild = server->GetMsgId();

				MessagePack(msgIdChild, MsgDeal::Req, G2L_ReqClientLogin::GetDescriptor()->full_name().c_str(), binData);

				ServerEntityHelper* serverEntityHelper = static_cast<ServerEntityHelper*>(serverEntity);

				{
					auto taskGen = [](Message* msg) -> DNTask<Message*>
						{
							co_return msg;
						};
					auto dataChannel = taskGen(&response);
					server->AddMsg(msgIdChild, &dataChannel, 9000);
					serverEntityHelper->GetSock()->write(binData);
					co_await dataChannel;
					if (dataChannel.HasFlag(DNTaskFlag::Timeout))
					{
						response.set_state_code(4);
						DNPrint(0, LoggerLevel::Debug, "requst timeout! ");
					}

				}
			}
			
		}

		binData.clear();
		binData.resize(response.ByteSizeLong());
		response.SerializeToArray(binData.data(), binData.size());

		MessagePack(msgId, MsgDeal::Res, nullptr, binData);

		channel->write(binData);

		co_return;
	}
}
