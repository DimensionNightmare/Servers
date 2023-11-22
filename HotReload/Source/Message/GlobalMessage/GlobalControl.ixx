module;

#include "GlobalControl.pb.h"
#include "hv/Channel.h"
#include "hv/htime.h"

#include <coroutine>
export module GlobalControl;

import DNTask;
import DNServer;
//import GlobalServer;
import MessagePack;

using namespace GMsg::GlobalControl;
using namespace std;
using namespace google::protobuf;

// client request
export DNTaskVoid Msg_RegistSrv(int serverType, DNClientProxy* client, DNServerProxy* server)
{
	auto msgId = client->GetMsgId();
	auto reqMap = client->GetMsgMap();

	G2C_RegistSrv requset;
	requset.set_server_type(serverType);
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
	if(reqMap->contains(msgId))
	{
		cout << "+++++++++++++++++++++++++++++++++++++++" <<endl;
	}
	reqMap->emplace(msgId, &dataChannel);
	
	// wait data parse
	client->send(binData);
	co_await dataChannel;
	
	if(!response.success())
	{
		cout << "regist Server error" << endl;
	}
	else{
		cout << "regist Server success" << endl;
		client->SetRegisted(true);
	}

	dataChannel.Destroy();

	co_return;
}