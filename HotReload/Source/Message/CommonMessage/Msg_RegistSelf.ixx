module;

#include "hv/Channel.h"
#include "Common.pb.h"

#include <coroutine>
export module Msg_RegistSelf;

import DNTask;
import DNServer;
import MessagePack;

using namespace GMsg::Common;
using namespace std;
using namespace google::protobuf;

// client request
export void Msg_RegistSelf(int serverType, DNClientProxy& client, DNServerProxy& server)
{

	COM_RegistSelf Request;

	Request.set_server_type(serverType);
	Request.set_ip(server.host);
	Request.set_port(server.port);

	//client
	auto dataChannel = []()->DNTask<COM_RegistInfo*>
	{
		co_return new COM_RegistInfo;
	}();

	auto taskChannel = [&dataChannel]()-> DNTaskVoid
	{
		
		co_await dataChannel;

		co_return;
	}();

	string binData;
	binData.resize(Request.ByteSize());
	Request.SerializeToArray(binData.data(), binData.size());
	// task
	auto msgId = client.GetMsgId();
	client.GetMsgMap()->emplace(msgId, make_pair(&taskChannel,  (DNTask<Message*>*)&dataChannel));
	MessagePack(msgId, binData);
	client.send(binData);
}