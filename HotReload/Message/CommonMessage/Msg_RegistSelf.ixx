module;

#include "hv/Channel.h"
#include "Common.pb.h"

#include <coroutine>
export module Msg_RegistSelf;

import DNTask;
import BaseServer;

using namespace GMsg::Common;

export DNTaskVoid Msg_RegistSelf(int serverType, DnServer& server)
{
	COM_RegistSelf Request;
	COM_RegistInfo Response;

	Request.set_server_type(serverType);
	Request.set_ip(server.host);
	Request.set_port(server.port);
	

	auto handle = [Response]()-> DNTask<COM_RegistInfo>
	{
		co_return Response;
	};

	Response = co_await handle();
		
	co_return;
}