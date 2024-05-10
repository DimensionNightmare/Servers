module;
#include <coroutine>
#include <cstdint>
#include "hv/Channel.h"

#include "StdAfx.h"
#include "Server/S_Common.pb.h"
export module GlobalMessage:GlobalCommon;

import DNTask;
import MessagePack;
import GlobalServerHelper;

using namespace hv;
using namespace std;
using namespace google::protobuf;
using namespace GMsg;

namespace GlobalMessage
{

	// client request
	export DNTaskVoid Evt_ReqRegistSrv()
	{
		GlobalServerHelper* dnServer = GetGlobalServer();
		DNClientProxyHelper* client = dnServer->GetCSock();
		DNServerProxyHelper* server = dnServer->GetSSock();
		uint32_t msgId = client->GetMsgId();
		
		// first Can send Msg?
		if(client->GetMsg(msgId))
		{
			DNPrint(0, LoggerLevel::Debug, "+++++ %lu, \n", msgId);
			co_return;
		}
		else
		{
			DNPrint(0, LoggerLevel::Debug, "Evt_ReqRegistSrv ----- %lu, \n", msgId);
		}

		client->RegistState() = RegistState::Registing;

		COM_ReqRegistSrv requset;
		requset.set_server_type((int)dnServer->GetServerType());
		if(server->host == "0.0.0.0")
		{
			requset.set_ip("127.0.0.1");
		}
		else
		{
			requset.set_ip(server->host);
		}
		requset.set_port(server->port);
		
		// pack data
		string binData;
		binData.resize(requset.ByteSizeLong());
		requset.SerializeToArray(binData.data(), binData.size());
		MessagePack(msgId, MsgDeal::Req, requset.GetDescriptor()->full_name().c_str(), binData);
		
		// data alloc
		COM_ResRegistSrv response;
		auto taskGen = [&response]() -> DNTask<Message>
		{
			co_return response;
		};

		auto dataChannel = taskGen();

		{
			// wait data parse
			client->AddMsg(msgId, &dataChannel);
			client->send(binData);
			co_await dataChannel;
			if(dataChannel.HasFlag(DNTaskFlag::Timeout))
			{
				DNPrint(0, LoggerLevel::Debug, "requst timeout! \n");
			}
		}
		
		if(response.success())
		{
			DNPrint(0, LoggerLevel::Debug, "regist Server success! \n");
			client->RegistState() = RegistState::Registed;
			dnServer->ServerIndex() = response.server_index();
		}
		else
		{
			DNPrint(0, LoggerLevel::Debug, "regist Server error! msg:%lu \n", msgId);
			// dnServer->IsRun() = false; //exit application
			client->RegistState() = RegistState::None;
		}

		dataChannel.Destroy();
		co_return;
	}

	// client request
	export void Msg_ReqRegistSrv(const SocketChannelPtr &channel, uint32_t msgId, Message *msg)
	{
		COM_ReqRegistSrv* requset = reinterpret_cast<COM_ReqRegistSrv*>(msg);
		COM_ResRegistSrv response;

		GlobalServerHelper* dnServer = GetGlobalServer();
		ServerEntityManagerHelper*  entityMan = dnServer->GetServerEntityManager();

		ServerType regType = (ServerType)requset->server_type();
		
		if(regType < ServerType::GateServer || regType > ServerType::LogicServer)
		{
			response.set_success(false);
		}

		//exist?
		else if (ServerEntityHelper* entity = channel->getContext<ServerEntityHelper>())
		{
			response.set_success(false);
		}

		// take task to regist !
		else if (requset->server_index())
		{
			if (ServerEntityHelper* entity = entityMan->GetEntity(requset->server_index()))
			{
				// wait destroy`s destroy
				if (uint64_t timerId = entity->TimerId())
				{
					entity->TimerId() = 0;
					entityMan->Timer()->killTimer(timerId);
				}

				// already connect
				if(auto sock = entity->GetSock())
				{
					response.set_success(false);
				}
				else
				{
					entity->LinkNode() = nullptr;
					entity->SetSock(channel);
					channel->setContext(entity);

					response.set_success(true);

					// Re-enroll
					entityMan->MountEntity(regType, entity);
				}
			}
			else
			{
				response.set_success(true);
				response.set_server_index(requset->server_index());

				entity = entityMan->AddEntity(requset->server_index(), regType);
				entity->SetSock(channel);

				channel->setContext(entity);

				entity->ServerIp() = requset->ip();
				entity->ServerPort() = requset->port();
				
				for (int i = 0; i < requset->childs_size(); i++)
				{
					const COM_ReqRegistSrv& child = requset->childs(i);
					ServerType childType = (ServerType)child.server_type();
					ServerEntityHelper* servChild = entityMan->AddEntity(child.server_index(), childType);
					entity->SetMapLinkNode(childType, servChild);
				}
				

			}
			
		}

		else if (ServerEntityHelper* entity = entityMan->AddEntity(entityMan->ServerIndex(), regType))
		{
			entity->ServerIp() = requset->ip();
			entity->ServerPort() = requset->port();
			entity->SetSock(channel);

			channel->setContext(entity);

			response.set_success(true);
			response.set_server_index(entity->ID());
		}
		
		string binData;
		binData.resize(response.ByteSizeLong());
		response.SerializeToArray(binData.data(), binData.size());

		MessagePack(msgId, MsgDeal::Res, nullptr, binData);
		channel->write(binData);

		if(response.success())
		{
			dnServer->UpdateServerGroup();
		}

	}

	export void Exe_RetHeartbeat(const SocketChannelPtr &channel, uint32_t msgId, Message *msg)
	{
		COM_RetHeartbeat* requset = reinterpret_cast<COM_RetHeartbeat*>(msg);
	}
}
