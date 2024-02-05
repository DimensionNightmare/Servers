module;
#include "GateGlobal.pb.h"
#include "hv/Channel.h"

#include <coroutine>
export module GlobalMessage:GlobalGate;

import DNTask;
import MessagePack;
import GlobalServerHelper;
import ServerEntityHelper;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg::GateGlobal;

export void Exe_RetRegistSrv(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	G2G_RetRegistSrv* requset = (G2G_RetRegistSrv*)msg;

	auto entityMan = GetGlobalServer()->GetEntityManager();
	if(ServerEntityHelper* entity = entityMan->GetEntity(requset->server_index()))
	{
		if(uint64_t timerId = entity->GetChild()->GetTimerId())
		{
			entity->GetChild()->SetTimerId(0);
			GetGlobalServer()->GetSSock()->loop(0)->killTimer(timerId);
		}


	}
	
}