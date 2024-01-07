module;
#include "AuthControl.pb.h"

#include <coroutine>
export module AuthControl;

import DNTask;
import MessagePack;
import AuthServerHelper;

using namespace GMsg::AuthControl;
using namespace std;
using namespace google::protobuf;

// client request
export DNTaskVoid Msg_RegistSrv()
{
	auto AuthServer = GetAuthServer();
	auto client = AuthServer->GetCSock();
	auto server = AuthServer->GetSSock();
	auto msgId = client->GetMsgId();
	auto& reqMap = client->GetMsgMap();
	
	// first Can send Msg?
	if(reqMap.contains(msgId))
	{
		printf("%s->+++++ %d, \n", __FUNCTION__, msgId);
		co_return;
	}
	else
	{
		printf("%s->----- %d, \n", __FUNCTION__, msgId);
	}

	A2C_RegistSrv requset;
	requset.set_server_type((int)AuthServer->GetServerType());
	requset.set_ip(server->host);
	requset.set_port(server->port);
	
	// pack data
	string binData;
	binData.resize(requset.ByteSizeLong());
	requset.SerializeToArray(binData.data(), (int)binData.size());
	MessagePack(msgId, MsgDeal::Req, requset.GetDescriptor()->full_name(), binData);
	
	// data alloc
	C2A_RegistSrv response;
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
		printf("%s->regist Server error! msg:%d \n", __FUNCTION__, msgId);
	}
	else
	{
		printf("%s->regist Server success! \n", __FUNCTION__);
		client->SetRegisted(true);
	}

	dataChannel.Destroy();

	co_return;
}