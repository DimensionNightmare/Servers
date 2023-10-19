module;

#include "hv/Channel.h"
#include "Common.pb.h"

#include <coroutine>
#include <map>
export module Msg_RegistSelf;

import DNTask;
import DNServer;

using namespace GMsg::Common;
using namespace std;

// client request
export void Msg_RegistSelf(DNClientProxy& client, int serverType, DNServerProxy& server)
{
	auto handle = [&serverType, &server]()-> DNTaskVoid
	{
		COM_RegistSelf Request;

		Request.set_server_type(serverType);
		Request.set_ip(server.host);
		Request.set_port(server.port);

		//client

		auto Res = []()->DNTask<COM_RegistInfo*>
		{
			co_return new COM_RegistInfo;
		};

		auto resHandle = Res();

		co_await resHandle;

		co_return;
	}();

	// task
	client.InsertMsg(client.GetMsgId(), &handle);

}