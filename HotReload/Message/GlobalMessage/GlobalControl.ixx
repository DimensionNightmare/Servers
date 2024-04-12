module;
#include "StdAfx.h"
#include "S_Common.pb.h"

#include <coroutine>
export module GlobalMessage:GlobalControl;

import DNTask;
import MessagePack;
import GlobalServerHelper;


using namespace std;
using namespace google::protobuf;
using namespace GMsg::S_Common;

// client request
export DNTaskVoid Evt_ReqRegistSrv()
{
	GlobalServerHelper* dnServer = GetGlobalServer();
	DNClientProxyHelper* client = dnServer->GetCSock();
	DNServerProxyHelper* server = dnServer->GetSSock();
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
		// dnServer->IsRun() = false; //exit application
	}
	else
	{
		DNPrint(-1, LoggerLevel::Debug, "regist Server success! \n");
		client->SetRegisted(true);
		dnServer->ServerIndex() = response.server_index();
	}

	dataChannel.Destroy();
	client->SetIsRegisting(false);
	co_return;
}