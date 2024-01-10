module;
#include "Common.pb.h"
#include "GlobalControl.pb.h"

#include <coroutine>
export module GlobalControl;

import DNTask;
import MessagePack;
import GlobalServerHelper;
import AfxCommon;

#define DNPrint(fmt, ...) printf("[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr(), __FUNCTION__, ##__VA_ARGS__);
#define DNPrintErr(fmt, ...) fprintf(stderr, "[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr(), __FUNCTION__, ##__VA_ARGS__);

using namespace std;
using namespace google::protobuf;
using namespace GMsg::Common;

// client request
export DNTaskVoid Msg_RegistSrv()
{
	auto globalServer = GetGlobalServer();
	auto client = globalServer->GetCSock();
	auto server = globalServer->GetSSock();
	auto msgId = client->GetMsgId();
	auto& reqMap = client->GetMsgMap();
	
	// first Can send Msg?
	if(reqMap.contains(msgId))
	{
		DNPrintErr("+++++ %lu, \n", msgId);
		co_return;
	}
	else
	{
		DNPrint("----- %lu, \n", msgId);
	}

	COM_ReqRegistSrv requset;
	requset.set_server_type((int)globalServer->GetServerType());
	requset.set_ip(server->host);
	requset.set_port(server->port);
	
	// pack data
	string binData;
	binData.resize(requset.ByteSizeLong());
	requset.SerializeToArray(binData.data(), (int)binData.size());
	MessagePack(msgId, MsgDeal::Req, requset.GetDescriptor()->full_name(), binData);
	
	// data alloc
	COM_ResRegistSrv response;
	auto dataChannel = [&]()->DNTask<Message*>
	{
		co_return &response;
	}();

	

	reqMap.emplace(msgId, (DNTask<void*>*)&dataChannel);
	
	// wait data parse
	client->send(binData);
	co_await dataChannel;
	
	if(!response.success())
	{
		DNPrint("regist Server error! msg:%lu \n", msgId);
	}
	else
	{
		DNPrint("regist Server success! \n");
		client->SetRegisted(true);
	}

	dataChannel.Destroy();

	co_return;
}