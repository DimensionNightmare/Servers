module;
#include "StdAfx.h"
#include "S_Logic.pb.h"
#include "hv/Channel.h"

#include <coroutine>
export module LogicMessage:LogicGate;

import DNTask;
import MessagePack;
import LogicServerHelper;


using namespace std;
using namespace google::protobuf;
using namespace hv;
using namespace GMsg::S_Logic;

// client request
export void Exe_RetClientLogin(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	G2L_RetClientLogin* requset = reinterpret_cast<G2L_RetClientLogin*>(msg);

	auto entityMan = GetLogicServer()->GetClientEntityManager();

	if (ClientEntityHelper* entity = entityMan->AddEntity(requset->account_id()))
	{
		
	}
}
