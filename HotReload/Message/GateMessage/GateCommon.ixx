module;
#include <coroutine>
#include <cstdint>
#include <list>
#include "hv/Channel.h"

#include "StdAfx.h"
#include "Server/S_Common.pb.h"
#include "Server/S_Global_Gate.pb.h"
export module GateMessage:GateCommon;

import DNTask;
import MessagePack;
import GateServerHelper;

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

		COM_ReqRegistSrv requset;
		requset.set_server_type((int)dnServer->GetServerType());

		requset.set_port(server->port);
		requset.set_server_index(dnServer->ServerIndex());

		ServerEntityManagerHelper* entityMan = dnServer->GetServerEntityManager();
		auto AddChild = [&requset](ServerEntity* serv)
			{
				COM_ReqRegistSrv* child = requset.add_childs();
				ServerEntityHelper* helper = static_cast<ServerEntityHelper*>(serv);
				child->set_server_index(helper->ID());
				ServerType servType = helper->ServerEntityType();
				child->set_server_type((uint32_t)servType);
			};

		list<ServerEntity*>& dbs = entityMan->GetEntityByList(ServerType::DatabaseServer);
		for (ServerEntity* serv : dbs)
		{
			AddChild(serv);
		}

		list<ServerEntity*>& logics = entityMan->GetEntityByList(ServerType::LogicServer);
		for (ServerEntity* serv : logics)
		{
			AddChild(serv);
		}

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
			DNPrint(0, LoggerLevel::Debug, "regist Server success! ");
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

		GateServerHelper* dnServer = GetGateServer();
		ServerEntityManagerHelper* entityMan = dnServer->GetServerEntityManager();

		ServerType regType = (ServerType)requset->server_type();

		const string& ipPort = channel->localaddr();

		if (regType < ServerType::DatabaseServer || regType > ServerType::LogicServer || ipPort.empty())
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
			binData.clear();

			// up to Global
			g2G_RetRegistSrv retMsg;
			retMsg.set_is_regist(true);
			retMsg.set_server_index(requset->server_index());

			binData.clear();
			binData.resize(retMsg.ByteSizeLong());
			retMsg.SerializeToArray(binData.data(), binData.size());
			MessagePack(0, MsgDeal::Ret, retMsg.GetDescriptor()->full_name().c_str(), binData);

			GateServerHelper* dnServer = GetGateServer();
			DNClientProxyHelper* client = dnServer->GetCSock();
			client->send(binData);
		}
	}

	export void Exe_RetHeartbeat(SocketChannelPtr channel, uint32_t msgId, Message* msg)
	{
		COM_RetHeartbeat* requset = reinterpret_cast<COM_RetHeartbeat*>(msg);
	}
}
