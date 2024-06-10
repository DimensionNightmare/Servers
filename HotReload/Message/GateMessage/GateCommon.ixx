module;
#include <coroutine>
#include <cstdint>
#include <list>
#include "hv/Channel.h"

#include "StdMacro.h"
#include "Server/S_Common.pb.h"
#include "Server/S_Gate.pb.h"
export module GateMessage:GateCommon;

import DNTask;
import MessagePack;
import GateServerHelper;
import Logger;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg;

namespace GateMessage
{

	// self request
	export DNTaskVoid Evt_ReqRegistSrv()
	{
		GateServerHelper* dnServer = GetGateServer();
		DNClientProxyHelper* client = dnServer->GetCSock();
		DNServerProxy* server = dnServer->GetSSock();
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

		request.set_port(server->port);

		ServerEntityManagerHelper* entityMan = dnServer->GetServerEntityManager();
		auto AddChild = [&request](ServerEntity* serv)
			{
				COM_ReqRegistSrv* child = request.add_childs();
				child->set_server_index(serv->ID());
				child->set_server_type((uint32_t)serv->GetServerType());
			};

		const list<ServerEntity*>& dbs = entityMan->GetEntityByList(ServerType::DatabaseServer);
		for (ServerEntity* serv : dbs)
		{
			AddChild(serv);
		}

		const list<ServerEntity*>& logics = entityMan->GetEntityByList(ServerType::LogicServer);
		for (ServerEntity* serv : logics)
		{
			AddChild(serv);
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

		GateServerHelper* dnServer = GetGateServer();
		ServerEntityManagerHelper* entityMan = dnServer->GetServerEntityManager();

		ServerType regType = (ServerType)request->server_type();
		uint32_t serverIndex = request->server_index();

		const string& ipPort = channel->localaddr();

		if (regType < ServerType::DatabaseServer || regType > ServerType::LogicServer || ipPort.empty())
		{
			response.set_success(false);
		}

		//exist?
		if (ServerEntity* entity = channel->getContext<ServerEntity>())
		{
			response.set_success(false);
		}

		else if (ServerEntity* entity = entityMan->AddEntity(serverIndex, regType))
		{
			size_t pos = ipPort.find(":");
			entity->ServerIp() = ipPort.substr(0, pos);
			entity->ServerPort() = request->port();
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

		if (response.success())
		{
			// up to Global
			g2G_RetRegistSrv request;
			request.set_is_regist(true);
			request.set_server_index(serverIndex);

			request.SerializeToString(&binData);
			MessagePack(0, MsgDeal::Ret, request.GetDescriptor()->full_name().c_str(), binData);

			GateServerHelper* dnServer = GetGateServer();
			DNClientProxyHelper* client = dnServer->GetCSock();
			client->send(binData);
		}
	}

	export void Exe_RetHeartbeat(SocketChannelPtr channel, Message* msg)
	{
		COM_RetHeartbeat* request = reinterpret_cast<COM_RetHeartbeat*>(msg);
	}
}
