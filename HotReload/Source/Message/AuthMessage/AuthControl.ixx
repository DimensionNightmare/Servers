module;
#include "Common.pb.h"
#include "AuthControl.pb.h"

#include <coroutine>
export module AuthMessage:AuthControl;

import DNTask;
import MessagePack;
import AuthServerHelper;
import AfxCommon;

#define DNPrint(fmt, ...) printf("[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr().c_str(), __FUNCTION__, ##__VA_ARGS__);
#define DNPrintErr(fmt, ...) fprintf(stderr, "[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr().c_str(), __FUNCTION__, ##__VA_ARGS__);

using namespace std;
using namespace google::protobuf;
using namespace GMsg::Common;

// client request
export DNTaskVoid Msg_RegistSrv()
{
	auto AuthServer = GetAuthServer();
	auto client = AuthServer->GetCSock();
	auto server = AuthServer->GetSSock();
	auto msgId = client->GetMsgId();
	
	// first Can send Msg?
	if(client->GetMsg(msgId))
	{
		DNPrintErr("+++++ %lu, \n", msgId);
		co_return;
	}
	// else
	// {
	// 	printf("----- %lu, \n", msgId);
	// }

	COM_ReqRegistSrv requset;
	requset.set_server_type((int)AuthServer->GetServerType());
	requset.set_ip(server->host);
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

	
	client->AddMsg(msgId, (DNTask<void*>*)&dataChannel);
	
	// wait data parse
	client->send(binData);
	co_await dataChannel;
	
	if(!response.success())
	{
		DNPrint("regist Server error! msg:%lu \n", msgId);
		AuthServer->SetRun(false); //exit application
	}
	else
	{
		DNPrint("regist Server success! \n");
		client->SetRegisted(true);
	}

	dataChannel.Destroy();

	co_return;
}