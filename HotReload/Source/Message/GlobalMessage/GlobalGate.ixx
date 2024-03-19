module;
#include "S_Global.pb.h"
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
using namespace GMsg::S_Global;

export void Exe_RetRegistSrv(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	G2G_RetRegistSrv* requset = (G2G_RetRegistSrv*)msg;

	GlobalServerHelper* dnServer = GetGlobalServer();
	auto entityMan = dnServer->GetEntityManager();
	if(ServerEntityHelper* entity = entityMan->GetEntity(requset->server_index()))
	{
		if(requset->is_regist())
		{
			if(uint64_t timerId = entity->GetChild()->TimerId())
			{
				entity->GetChild()->TimerId() = 0;
				dnServer->GetSSock()->Timer()->killTimer(timerId);
			}
		}
		else
		{
			ServerEntityHelper* owner = channel->getContext<ServerEntityHelper>();
			// remove and unlock
			owner->GetMapLinkNode(entity->ServerEntityType()).remove(entity);
			owner->ClearFlag(ServerEntityFlag::Locked);

			entityMan->RemoveEntity(requset->server_index());
			dnServer->UpdateServerGroup();
		}

	}
	
}
