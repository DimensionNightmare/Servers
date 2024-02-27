module;
#include "StdAfx.h"
#include "CommonMsg.pb.h"
#include "GateGlobal.pb.h"
#include "hv/Channel.h"

#include <coroutine>
export module GateMessage:GateCommon;

import DNTask;
import MessagePack;
import GateServerHelper;
import ServerEntityHelper;


using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg::CommonMsg;
using namespace GMsg::GateGlobal;

// client request
export DNTaskVoid Msg_RegistSrv()
{
	GateServerHelper* dnServer = GetGateServer();
	auto client = dnServer->GetCSock();
	auto server = dnServer->GetSSock();
	unsigned int msgId = client->GetMsgId();
	
	// first Can send Msg?
	if(client->GetMsg(msgId))
	{
		DNPrint(-1, LoggerLevel::Error, "+++++ %lu, \n", msgId);
		co_return;
	}
	else
	{
		DNPrint(-1, LoggerLevel::Debug, "Msg_RegistSrv ----- %lu, \n", msgId);
	}

	client->SetIsRegisting(true);

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
	requset.set_server_index(dnServer->GetServerIndex());

	auto entityMan = dnServer->GetEntityManager();
	auto AddChild = [&requset](ServerEntity* serv)
	{
		COM_ReqRegistSrv* child = requset.add_childs();
		ServerEntityHelper* helper = static_cast<ServerEntityHelper*>(serv);
		child->set_server_index(helper->GetChild()->GetID());
		child->set_server_type((uint32_t)helper->GetServerType());
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
	auto dataChannel = [&response]()->DNTask<Message*>
	{
		co_return &response;
	}();


	client->AddMsg(msgId, &dataChannel);
	
	// wait data parse
	client->send(binData);
	co_await dataChannel;
	
	if(!response.success())
	{
		DNPrint(-1, LoggerLevel::Debug, "regist Server error! msg:%lu \n", msgId);
		// dnServer->SetRun(false); //exit application
	}
	else
	{
		DNPrint(-1, LoggerLevel::Debug, "regist Server success! \n");
		client->SetRegisted(true);
		dnServer->SetServerIndex(response.server_index());
	}

	dataChannel.Destroy();
	client->SetIsRegisting(false);
	co_return;
}

void ServerEntityCloseEvent(Entity* entity)
{
	ServerEntityHelper* castObj = static_cast<ServerEntityHelper*>(entity);

	// up to Global
	string binData;
	G2G_RetRegistSrv upLoad;
	upLoad.set_server_index(castObj->GetChild()->GetID());
	upLoad.set_is_regist(false);
	binData.resize(upLoad.ByteSizeLong());
	upLoad.SerializeToArray(binData.data(), (int)binData.size());
	MessagePack(0, MsgDeal::Req, upLoad.GetDescriptor()->full_name().c_str(), binData);

	GateServerHelper* dnServer = GetGateServer();
	auto client = dnServer->GetCSock();
	client->send(binData);
	
	auto entityMan = dnServer->GetEntityManager();
	entityMan->RemoveEntity(castObj->GetChild()->GetID());
}

// client request
export void Exe_RegistSrv(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	COM_ReqRegistSrv* requset = (COM_ReqRegistSrv*)msg;
	COM_ResRegistSrv response;

	auto entityMan = GetGateServer()->GetEntityManager();

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
		entity->GetChild()->SetSock(channel);
		
		channel->setContext(entity);

		response.set_success(true);
		response.set_server_index(entity->GetChild()->GetID());

		entity->SetCloseEvent(ServerEntityCloseEvent);
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
		MessagePack(0, MsgDeal::Req, upLoad.GetDescriptor()->full_name().c_str(), binData);

		GateServerHelper* dnServer = GetGateServer();
		auto client = dnServer->GetCSock();
		client->send(binData);
	}
}