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
		if (client->GetMsg(msgId))
		{
			DNPrint(0, LoggerLevel::Debug, "+++++ %lu, ", msgId);
			co_return;
		}
		else
		{
			DNPrint(0, LoggerLevel::Debug, "Client:%s, port:%hu", client->remote_host.c_str(), client->remote_port);
		}

		client->RegistState() = RegistState::Registing;

		COM_ReqRegistSrv requset;

		requset.set_server_type((int)dnServer->GetServerType());

		if (uint32_t serverIndex = dnServer->ServerIndex())
		{
			requset.set_server_index(serverIndex);
		}

		requset.set_port(server->port);

		// pack data
		string binData;
		binData.resize(requset.ByteSizeLong());
		requset.SerializeToArray(binData.data(), binData.size());
		MessagePack(msgId, MsgDeal::Req, requset.GetDescriptor()->full_name().c_str(), binData);

		// data alloc
		COM_ResRegistSrv response;

		{
			auto taskGen = [](Message* msg) -> DNTask<Message*>
				{
					co_return msg;
				};
			auto dataChannel = taskGen(&response);
			// wait data parse
			client->AddMsg(msgId, &dataChannel);
			client->send(binData);
			co_await dataChannel;
			if (dataChannel.HasFlag(DNTaskFlag::Timeout))
			{
				DNPrint(0, LoggerLevel::Debug, "requst timeout! ");
			}

		}

		if (response.success())
		{
			DNPrint(0, LoggerLevel::Debug, "regist Server success! Rec index:%d", response.server_index());
			client->RegistState() = RegistState::Registed;
			dnServer->ServerIndex() = response.server_index();
		}
		else
		{
			DNPrint(0, LoggerLevel::Debug, "regist Server error! msg:%lu ", msgId);
			// dnServer->IsRun() = false; //exit application
			client->RegistState() = RegistState::None;
		}


		co_return;
	}

	// client request
	export void Msg_ReqRegistSrv(SocketChannelPtr channel, uint32_t msgId, Message* msg)
	{
		COM_ReqRegistSrv* requset = reinterpret_cast<COM_ReqRegistSrv*>(msg);
		COM_ResRegistSrv response;

		GlobalServerHelper* dnServer = GetGlobalServer();
		ServerEntityManagerHelper* entityMan = dnServer->GetServerEntityManager();

		ServerType regType = (ServerType)requset->server_type();

		const string& ipPort = channel->localaddr();

		if (regType < ServerType::GateServer || regType > ServerType::LogicServer || ipPort.empty())
		{
			response.set_success(false);
		}

		//exist?
		else if (ServerEntityHelper* entity = channel->getContext<ServerEntityHelper>())
		{
			response.set_success(false);
		}

		// take task to regist !
		else if (int serverIndex = requset->server_index())
		{
			if (ServerEntityHelper* entity = entityMan->GetEntity(serverIndex))
			{
				// wait destroy`s destroy
				if (uint64_t timerId = entity->TimerId())
				{
					entity->TimerId() = 0;
					entityMan->Timer()->killTimer(timerId);
				}

				// already connect
				if (const SocketChannelPtr& sock = entity->GetSock())
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
				response.set_server_index(serverIndex);

				entity = entityMan->AddEntity(serverIndex, regType);
				entity->SetSock(channel);

				channel->setContext(entity);

				size_t pos = ipPort.find(":");
				entity->ServerIp() = ipPort.substr(0, pos);
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
			size_t pos = ipPort.find(":");
			entity->ServerIp() = ipPort.substr(0, pos);
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

		if (response.success())
		{
			dnServer->UpdateServerGroup();
		}

	}

	export void Exe_RetHeartbeat(SocketChannelPtr channel, uint32_t msgId, Message* msg)
	{
		COM_RetHeartbeat* requset = reinterpret_cast<COM_RetHeartbeat*>(msg);
	}
}
