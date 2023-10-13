module;

#include "hv/Channel.h"
#include "Common.pb.h"

#include <coroutine>
#include <map>
export module Msg_RegistSelf;

import DNTask;
import BaseServer;

using namespace GMsg::Common;
using namespace std;

// client request
export void Msg_RegistSelf(int serverType, DnServer& server)
{

	
	COM_RegistSelf Request;
	COM_RegistInfo Response;

	Request.set_server_type(serverType);
	Request.set_ip(server.host);
	Request.set_port(server.port);
	
	auto handle = [Response]()-> DNTaskVoid
	{
		co_return;
	}();


	map<int, DNTaskVoid*> tasks;
	tasks.emplace(3, &handle);

}