module;
#include "Common.pb.h"
#include "hv/Channel.h"

#include <coroutine>
export module DatabaseMessage:DatabaseCommon;

import DNTask;
import MessagePack;
import DatabaseServerHelper;
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
	DatabaseServerHelper* dnServer = GetDatabaseServer();
	auto client = dnServer->GetCSock();
	unsigned int msgId = client->GetMsgId();
	
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
		client->SetRegisted(true);
		dnServer->SetServerIndex(response.server_index());
	}

	dataChannel.Destroy();

	co_return;
}

export void Exe_RetChangeCtlSrv(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	COM_RetChangeCtlSrv* requset = (COM_RetChangeCtlSrv*)msg;
	DatabaseServerHelper* dnServer = GetDatabaseServer();
	auto client = dnServer->GetCSock();

	client->UpdateClientState(Channel::Status::CLOSED);

	GetClientReconnectFunc()(requset->ip().c_str(), requset->port());
}