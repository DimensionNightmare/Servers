module;
#include <coroutine>
#include <cstdint>
#include "hv/Channel.h"

#include "StdAfx.h"
#include "Server/S_Common.pb.h"
export module LogicMessage:LogicCommon;

import DNTask;
import MessagePack;
import LogicServerHelper;

using namespace std;
using namespace google::protobuf;
using namespace GMsg;
using namespace hv;

namespace LogicMessage
{

	// client request
	export DNTaskVoid Evt_ReqRegistSrv()
	{
		LogicServerHelper* dnServer = GetLogicServer();
		DNClientProxyHelper* client = dnServer->GetCSock();
		uint32_t msgId = client->GetMsgId();
		
		// first Can send Msg?
		if(client->GetMsg(msgId))
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
		requset.set_server_index(dnServer->ServerIndex());
		
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
				DNPrint(0, LoggerLevel::Debug, "requst timeout! ");
			}
		}
		
		if(response.success())
		{
			DNPrint(0, LoggerLevel::Debug, "regist Server success! ");
			client->RegistState() = RegistState::Registed;
			dnServer->ServerIndex() = response.server_index();
		}
		else
		{
			DNPrint(0, LoggerLevel::Debug, "regist Server error! msg:%lu ", msgId);
			dnServer->IsRun() = false; //exit application
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

		ServerEntityManagerHelper* entityMan = GetLogicServer()->GetServerEntityManager();

		ServerType regType = (ServerType)requset->server_type();
		
		if(regType != ServerType::DedicatedServer)
		{
			response.set_success(false);
		}

		//exist?
		if (ServerEntityHelper* entity = channel->getContext<ServerEntityHelper>())
		{
			response.set_success(false);
		}

		else if (ServerEntityHelper* entity = entityMan->AddEntity(requset->server_index(), regType))
		{
			entity->ServerIp() = requset->ip();
			entity->ServerPort() = requset->port();

			DNPrint(0, LoggerLevel::Debug, "ds regist:%s:%d", entity->ServerIp().c_str(), entity->ServerPort());

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
	}

	export void Exe_RetChangeCtlSrv(const SocketChannelPtr &channel, uint32_t msgId, Message *msg)
	{
		COM_RetChangeCtlSrv* requset = reinterpret_cast<COM_RetChangeCtlSrv*>(msg);
		LogicServerHelper* dnServer = GetLogicServer();

		dnServer->ReClientEvent(requset->ip(), requset->port());
	}

	export void Exe_RetHeartbeat(const SocketChannelPtr &channel, uint32_t msgId, Message *msg)
	{
		COM_RetHeartbeat* requset = reinterpret_cast<COM_RetHeartbeat*>(msg);
	}
}