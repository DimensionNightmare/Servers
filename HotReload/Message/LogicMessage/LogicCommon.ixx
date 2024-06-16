module;
#include <coroutine>
#include <cstdint>
#include <string>
#include <memory>

#include "StdMacro.h"
export module LogicMessage:LogicCommon;

import DNTask;
import MessagePack;
import LogicServerHelper;
import Logger;
import Macro;
import ThirdParty.Libhv;

namespace LogicMessage
{

	// client request
	export DNTaskVoid Evt_ReqRegistSrv()
	{
		LogicServerHelper* dnServer = GetLogicServer();
		DNClientProxyHelper* client = dnServer->GetCSock();
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

		COM_ReqRegistSrv request;

		request.set_server_type((int)dnServer->GetServerType());

		if (uint32_t serverIndex = dnServer->ServerIndex())
		{
			request.set_server_index(serverIndex);
		}

		// pack data
		string binData;
		request.SerializeToString(&binData);
		MessagePack(msgId, MsgDeal::Req, request.GetDescriptor()->full_name().c_str(), binData);

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
			client->RegistType() = response.server_type();
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
		COM_ReqRegistSrv* request = reinterpret_cast<COM_ReqRegistSrv*>(msg);
		COM_ResRegistSrv response;

		LogicServerHelper* dnServer = GetLogicServer();
		ServerEntityManagerHelper* entityMan = dnServer->GetServerEntityManager();

		ServerType regType = (ServerType)request->server_type();
		const string& ipPort = channel->localaddr();

		if (regType != ServerType::DedicatedServer || ipPort.empty())
		{
			response.set_success(false);
		}

		//exist?
		if (ServerEntity* entity = channel->getContext<ServerEntity>())
		{
			response.set_success(false);
		}

		else if (int serverIndex = request->server_index())
		{
			if (ServerEntity* entity = entityMan->GetEntity(serverIndex))
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
				response.set_server_type((uint8_t(dnServer->GetServerType())));

				entity = entityMan->AddEntity(serverIndex, regType);
				entity->SetSock(channel);

				channel->setContext(entity);

				size_t pos = ipPort.find(":");
				entity->ServerIp() = ipPort.substr(0, pos);
				entity->ServerPort() = request->port();
			}
		}

		else if (ServerEntity* entity = entityMan->AddEntity(entityMan->ServerIndex(), regType))
		{
			size_t pos = ipPort.find(":");
			entity->ServerIp() = ipPort.substr(0, pos);
			entity->ServerPort() = request->port();

			DNPrint(0, LoggerLevel::Debug, "ds regist:%s:%d", entity->ServerIp().c_str(), entity->ServerPort());

			entity->SetSock(channel);

			channel->setContext(entity);

			response.set_success(true);
			response.set_server_index(entity->ID());
			response.set_server_type((uint8_t(dnServer->GetServerType())));
		}

		string binData;
		response.SerializeToString(&binData);

		MessagePack(msgId, MsgDeal::Res, nullptr, binData);
		channel->write(binData);
	}

	export void Exe_RetChangeCtlSrv(SocketChannelPtr channel, Message* msg)
	{
		COM_RetChangeCtlSrv* request = reinterpret_cast<COM_RetChangeCtlSrv*>(msg);
		LogicServerHelper* dnServer = GetLogicServer();
		DNClientProxyHelper* client = dnServer->GetCSock();

		TICK_MAINSPACE_SIGN_FUNCTION(DNClientProxy, RedirectClient, client, request->port(), request->ip());
	}

	export void Exe_RetHeartbeat(SocketChannelPtr channel, Message* msg)
	{
		COM_RetHeartbeat* request = reinterpret_cast<COM_RetHeartbeat*>(msg);
	}
}