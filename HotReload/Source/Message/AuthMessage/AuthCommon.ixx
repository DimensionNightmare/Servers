module;
#include "StdAfx.h"
#include "S_Common.pb.h"

#include <coroutine>
export module AuthMessage:AuthCommon;

import DNTask;
import MessagePack;
import AuthServerHelper;


using namespace std;
using namespace google::protobuf;
using namespace GMsg::S_Common;

// client request
export DNTaskVoid Msg_RegistSrv()
{
	AuthServerHelper* dnServer = GetAuthServer();
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
		// prinfg("Msg_RegistSrv ----- %lu, \n", msgId);
	}

	client->SetIsRegisting(true);

	COM_ReqRegistSrv requset;
	requset.set_server_type((int)dnServer->GetServerType());
	if(!strcmp(server->host, "0.0.0.0"))
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
		DNPrint(-1, LoggerLevel::Debug, "regist Server success! \n");
		client->SetRegisted(true);
		dnServer->SetServerIndex(response.server_index());
	}

	dataChannel.Destroy();

	client->SetIsRegisting(false);
	co_return;
}