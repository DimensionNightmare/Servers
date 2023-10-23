module;

#include "hv/Channel.h"
#include "GlobalControl.pb.h"

#include <coroutine>
export module GlobalControl;

import DNTask;
import DNServer;
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
	MessagePack(msgId, MsgDir::Inner, G2C_RegistSrv::GetDescriptor()->full_name(), binData);
	
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
	reqMap->erase(msgId);
	
	if(!response.success())
	{
		cout << "regist false" << (int)msgId << endl;
		Msg_RegistSrv(serverType, client, server); //error tick
	}
	else{
		cout << "regist true" << (int)msgId << endl;
	}
	dataChannel.Destroy();

	co_return;
}