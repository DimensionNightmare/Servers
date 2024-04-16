module;
#include "StdAfx.h"
#include "S_Common.pb.h"
#include "hv/Channel.h"

#include <coroutine>
export module LogicMessage:LogicCommon;

import DNTask;
import MessagePack;
import LogicServerHelper;


using namespace std;
using namespace google::protobuf;
using namespace GMsg::S_Common;
using namespace hv;

// client request
export DNTaskVoid Evt_ReqRegistSrv()
{
	LogicServerHelper* dnServer = GetLogicServer();
	DNClientProxyHelper* client = dnServer->GetCSock();
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
	requset.set_server_index(dnServer->ServerIndex());
	
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
	
	if(!response.success())
	{
		DNPrint(-1, LoggerLevel::Debug, "regist Server error! msg:%lu \n", msgId);
		dnServer->IsRun() = false; //exit application
		client->RegistState() = RegistState::None;
	}
	else
	{
		DNPrint(-1, LoggerLevel::Debug, "regist Server success! \n");
		client->RegistState() = RegistState::Registed;
		dnServer->ServerIndex() = response.server_index();
	}

	dataChannel.Destroy();
	co_return;
}

// client request
export void Msg_ReqRegistSrv(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	COM_ReqRegistSrv* requset = reinterpret_cast<COM_ReqRegistSrv*>(msg);
	COM_ResRegistSrv response;

	auto entityMan = GetLogicServer()->GetEntityManager();

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
		entity->GetChild()->SetSock(channel);
		
		channel->setContext(entity);

		response.set_success(true);
		response.set_server_index(entity->GetChild()->ID());
	}
	
	string binData;
	binData.resize(response.ByteSize());
	response.SerializeToArray(binData.data(), binData.size());

	MessagePack(msgId, MsgDeal::Res, nullptr, binData);
	channel->write(binData);
}

export void Exe_RetChangeCtlSrv(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	COM_RetChangeCtlSrv* requset = reinterpret_cast<COM_RetChangeCtlSrv*>(msg);
	LogicServerHelper* dnServer = GetLogicServer();
	DNClientProxyHelper* client = dnServer->GetCSock();

	client->UpdateClientState(Channel::Status::CLOSED);

	dnServer->ReClientEvent(requset->ip(), requset->port());
}

export void Exe_RetHeartbeat(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	COM_RetHeartbeat* requset = reinterpret_cast<COM_RetHeartbeat*>(msg);
}