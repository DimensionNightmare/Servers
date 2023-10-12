module;

#include "hv/Channel.h"
#include "Common.pb.h"

#include <coroutine>
export module Msg_RegistSelf;

import DNTask;

using namespace GMsg::Common;

void Msg_RegistSelf()
{
	static auto reply = []()-> DNTask<COM_RegistInfo*>
	{
		COM_RegistSelf regist;
		// regist.set_server_type((int)ServerType::GlobalServer);
		// if(auto sSock = server->GetSSock())
		// {
		// 	regist.set_ip(sSock->host);
		// 	regist.set_port(sSock->port);
		// }
		co_await std::suspend_always{};

		co_return;
	};

	reply();
}