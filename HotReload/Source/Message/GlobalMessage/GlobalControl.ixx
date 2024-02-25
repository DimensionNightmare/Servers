module;
#include "StdAfx.h"
#include "CommonMsg.pb.h"
#include "GlobalControl.pb.h"

#include <coroutine>
export module GlobalMessage:GlobalControl;

import DNTask;
import MessagePack;
import GlobalServerHelper;


using namespace std;
using namespace google::protobuf;
using namespace GMsg::CommonMsg;

// client request
export DNTaskVoid Msg_RegistSrv()
{
	GlobalServerHelper* dnServer = GetGlobalServer();
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
	
	// pack data
	string binData;
	binData.resize(requset.ByteSizeLong());
	requset.SerializeToArray(binData.data(), (int)binData.size());
	MessagePack(msgId, MsgDeal::Req, requset.GetDescriptor()->full_name(), binData);
	
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

	co_return;
}