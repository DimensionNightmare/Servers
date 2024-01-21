module;
#include "Common.pb.h"
#include "hv/Channel.h"

#include <coroutine>
export module LogicMessage:LogicCommon;

import DNTask;
import MessagePack;
import LogicServerHelper;
import AfxCommon;

#define DNPrint(fmt, ...) printf("[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr().c_str(), __FUNCTION__, ##__VA_ARGS__);
#define DNPrintErr(fmt, ...) fprintf(stderr, "[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr().c_str(), __FUNCTION__, ##__VA_ARGS__);

using namespace std;
using namespace google::protobuf;
using namespace GMsg::Common;
using namespace hv;

// client request
export DNTaskVoid Msg_RegistSrv()
{
	auto dnServer = GetLogicServer();
	auto client = dnServer->GetCSock();
	auto server = dnServer->GetSSock();
	auto msgId = client->GetMsgId();
	
	// first Can send Msg?
	if(client->GetMsg(msgId))
	{
		DNPrintErr("+++++ %lu, \n", msgId);
		co_return;
	}
	else
	{
		DNPrint("Msg_RegistSrv ----- %lu, \n", msgId);
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
	requset.set_server_index(dnServer->GetServerIndex());
	
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


	client->AddMsg(msgId, (DNTask<void*>*)&dataChannel);
	
	// wait data parse
	client->send(binData);
	co_await dataChannel;
	
	if(!response.success())
	{
		DNPrint("regist Server error! msg:%lu \n", msgId);
		// dnServer->SetRun(false); //exit application
	}
	else
	{
		DNPrint("regist Server success! \n");
		if(dnServer->GetServerIndex() == 0 && response.server_index())
		{
			
		}
		client->SetRegisted(true);
	}

	dataChannel.Destroy();

	co_return;
}

export void Exe_RetChangeCtlSrv(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	COM_RetChangeCtlSrv* requset = (COM_RetChangeCtlSrv*)msg;
	auto dnServer = GetLogicServer();
	auto client = dnServer->GetCSock();

	client->UpdateClientState(Channel::Status::CLOSED);

	GetClientReconnectFunc()(requset->ip().c_str(), requset->port());
}