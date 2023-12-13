module;

#include "GlobalControl.pb.h"
#include "hv/Channel.h"
#include "hv/htime.h"

#include <coroutine>
export module GlobalControl;

import DNTask;
import DNServer;
import MessagePack;
import GlobalServerHelper;

using namespace GMsg::GlobalControl;
using namespace std;
using namespace google::protobuf;

// client request
export DNTaskVoid Msg_RegistSrv(GlobalServerHelper* globalServer)
{
	auto client = globalServer->GetCSock();
	auto server = globalServer->GetSSock();
	auto msgId = client->GetMsgId();
	auto& reqMap = client->GetMsgMap();

	G2C_RegistSrv requset;
	requset.set_server_type((int)globalServer->GetServerType());
	requset.set_ip(server->host);
	requset.set_port(server->port);
	
	// pack data
	string binData;
	binData.resize(requset.ByteSize());
	requset.SerializeToArray(binData.data(), binData.size());
	MessagePack(msgId, MsgDeal::Req, requset.GetDescriptor()->full_name(), binData);
	
	// data alloc
	C2G_RegistSrv response;
	auto dataChannel = [&]()->DNTask<Message*>
		{
			co_return &response;
		}();
	if(reqMap.contains(msgId))
	{
		printf("%s->+++++++++++++++++++++++++++++++++++++++! \n", __FUNCTION__);
	}
	reqMap.emplace(msgId, &dataChannel);
	
	// wait data parse
	client->send(binData);
	co_await dataChannel;
	
	if(!response.success())
	{
		printf("%s->regist Server error! \n", __FUNCTION__);
	}
	else{
		printf("%s->regist Server success! \n", __FUNCTION__);
		client->SetRegisted(true);
	}

	dataChannel.Destroy();

	co_return;
}