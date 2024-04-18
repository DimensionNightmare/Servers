module;
#include "StdAfx.h"
#include "S_Common.pb.h"
#include "S_Global.pb.h"

#include "hv/Channel.h"
#include <coroutine>
export module GateMessage:GateCommon;

import DNTask;
import MessagePack;
import GateServerHelper;
import ServerEntityHelper;
import DNServerProxyHelper;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg::S_Common;
using namespace GMsg::S_Global;

// self request
export DNTaskVoid Evt_ReqRegistSrv()
{
	GateServerHelper* dnServer = GetGateServer();
	DNClientProxyHelper* client = dnServer->GetCSock();
	DNServerProxy* server = dnServer->GetSSock();
	unsigned int msgId = client->GetMsgId();
	
	// first Can send Msg?
	if(client->GetMsg(msgId))
	{
		DNPrint(-1, LoggerLevel::Error, "+++++ %lu, \n", msgId);
		co_return;
	}
	else
	{
		DNPrint(-1, LoggerLevel::Debug, "Evt_ReqRegistSrv ----- %lu, \n", msgId);
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
	requset.set_server_index(dnServer->ServerIndex());

	auto entityMan = dnServer->GetEntityManager();
	auto AddChild = [&requset](ServerEntity* serv)
	{
		COM_ReqRegistSrv* child = requset.add_childs();
		ServerEntityHelper* helper = static_cast<ServerEntityHelper*>(serv);
		child->set_server_index(helper->GetChild()->ID());
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
	requset.SerializeToArray(binData.data(), (int)binData.size());
	MessagePack(msgId, MsgDeal::Req, requset.GetDescriptor()->full_name().c_str(), binData);
	
	// data alloc
	COM_ResRegistSrv response;
	auto dataChannel = [&response]()->DNTask<Message>
	{
		co_return response;
	}();


	{
		// wait data parse
		client->AddMsg(msgId, &dataChannel);
		client->send(binData);
		co_await dataChannel;
		if(dataChannel.HasFlag(DNTaskFlag::Timeout))
		{

		}
	}
	
	if(response.success())
	{
		DNPrint(-1, LoggerLevel::Debug, "regist Server success! \n");
		client->RegistState() = RegistState::Registed;
		dnServer->ServerIndex() = response.server_index();
	}
	else
	{
		DNPrint(-1, LoggerLevel::Debug, "regist Server error! msg:%lu \n", msgId);
		dnServer->IsRun() = false; //exit application
		client->RegistState() = RegistState::None;
	}

	dataChannel.Destroy();
	co_return;
}

// client request
export void Msg_ReqRegistSrv(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	COM_ReqRegistSrv* requset = reinterpret_cast<COM_ReqRegistSrv*>(msg);
	COM_ResRegistSrv response;

	GateServerHelper* dnServer = GetGateServer();
	auto entityMan = dnServer->GetEntityManager();

	ServerType regType = (ServerType)requset->server_type();
	
	if(regType < ServerType::DatabaseServer || regType > ServerType::LogicServer)
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
		entity->GetChild()->SetSock(channel);
		
		channel->setContext(entity);

		response.set_success(true);
		response.set_server_index(entity->GetChild()->ID());

		entity->CloseEvent() = std::bind(&GateServerHelper::ServerEntityCloseEvent, dnServer, placeholders::_1);
	}
	
	string binData;
	binData.resize(response.ByteSize());
	response.SerializeToArray(binData.data(), binData.size());

	MessagePack(msgId, MsgDeal::Res, nullptr, binData);
	channel->write(binData);

	if(response.success())
	{
		binData.clear();

		// up to Global
		G2G_RetRegistSrv upLoad;
		upLoad.set_is_regist(true);
		upLoad.set_server_index(requset->server_index());
		binData.resize(upLoad.ByteSizeLong());
		upLoad.SerializeToArray(binData.data(), (int)binData.size());
		MessagePack(0, MsgDeal::Ret, upLoad.GetDescriptor()->full_name().c_str(), binData);

		GateServerHelper* dnServer = GetGateServer();
		DNClientProxyHelper* client = dnServer->GetCSock();
		client->send(binData);
	}
}

export void Exe_RetHeartbeat(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	COM_RetHeartbeat* requset = reinterpret_cast<COM_RetHeartbeat*>(msg);
}
