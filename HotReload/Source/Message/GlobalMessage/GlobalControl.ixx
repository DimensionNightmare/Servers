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
	auto dataFunc = []()->DNTask<Message*>
	{
		co_return new C2G_RegistSrv;
	};

	auto dataChannel = dataFunc();

	auto taskFunc = [](DNTask<Message*> dataChannel)-> DNTaskVoid
	{
		co_await suspend_always{};
		auto res = dataChannel.GetResult();
		cout << "success" << endl;
		dataChannel.tHandle.destroy();
		co_return;
	};

	auto taskChannel = taskFunc(dataChannel);

	string binData;
	binData.resize(requset.ByteSize());
	requset.SerializeToArray(binData.data(), binData.size());
	// task
	auto msgId = client.GetMsgId();
	client.GetMsgMap()->insert({msgId, {taskChannel,  dataChannel}});
	MessagePack(msgId, MsgDir::Inner, G2C_RegistSrv::GetDescriptor()->full_name(), binData);
	client.send(binData);
}