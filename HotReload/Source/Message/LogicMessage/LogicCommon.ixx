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

	client->SetIsRegisting(true);

	COM_ReqRegistSrv requset;
	requset.set_server_type((int)dnServer->GetServerType());
	requset.set_server_index(dnServer->GetServerIndex());
	
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
		// dnServer->SetRun(false); //exit application
	}
	else
	{
		DNPrint(-1, LoggerLevel::Debug,"regist Server success! \n");
		client->SetRegisted(true);
		dnServer->SetServerIndex(response.server_index());
	}

	dataChannel.Destroy();
	client->SetIsRegisting(false);
	co_return;
}

export void Exe_RetChangeCtlSrv(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	COM_RetChangeCtlSrv* requset = (COM_RetChangeCtlSrv*)msg;
	LogicServerHelper* dnServer = GetLogicServer();
	DNClientProxyHelper* client = dnServer->GetCSock();

	client->UpdateClientState(Channel::Status::CLOSED);

	GetClientReconnectFunc()(requset->ip().c_str(), requset->port());
}

// client request
export void Msg_ReqRegistSrv(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	COM_ReqRegistSrv* requset = (COM_ReqRegistSrv*)msg;
	COM_ResRegistSrv response;

	ServerEntityManagerHelper<ServerEntity>*  entityMan = GetLogicServer()->GetEntityManager();

	ServerType regType = (ServerType)requset->server_type();
	
	if(regType != ServerType::DedicatedServer)
	{
		response.set_success(false);
	}

	//exist?
	else if (ServerEntityHelper* entity = channel->getContext<ServerEntityHelper>())
	{
		response.set_success(false);
	}

	else if (ServerEntityHelper* entity = entityMan->AddEntity(requset->server_index(), regType))
	{
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

export void Exe_RetHeartbeat(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	COM_RetHeartbeat* requset = (COM_RetHeartbeat*)msg;
}
