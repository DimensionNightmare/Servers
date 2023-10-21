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
export void Msg_RegistSrv(int serverType, DNClientProxy& client, DNServerProxy& server)
{

	G2C_RegistSrv requset;

	requset.set_server_type(serverType);
	requset.set_ip(server.host);
	requset.set_port(server.port);

	//client
	auto dataChannel = []()->DNTask<C2G_RegistSrv*>
	{
		co_return new C2G_RegistSrv;
	}();

	auto taskChannel = [&dataChannel]()-> DNTaskVoid
	{
		
		co_await dataChannel;

		co_return;
	}();

	string binData;
	binData.resize(requset.ByteSize());
	requset.SerializeToArray(binData.data(), binData.size());
	// task
	auto msgId = client.GetMsgId();
	client.GetMsgMap()->emplace(msgId, make_pair(&taskChannel,  (DNTask<Message*>*)&dataChannel));
	MessagePack(msgId, MsgDir::Inner, G2C_RegistSrv::GetDescriptor()->full_name(), binData);
	client.send(binData);
}