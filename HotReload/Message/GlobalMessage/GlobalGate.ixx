module;
#include <coroutine>
#include "hv/Channel.h"

#include "Server/S_Global.pb.h"
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
	G2G_RetRegistSrv* requset = reinterpret_cast<G2G_RetRegistSrv*>(msg);

	GlobalServerHelper* dnServer = GetGlobalServer();
	ServerEntityManagerHelper<ServerEntity>*  entityMan = dnServer->GetServerEntityManager();
	if(ServerEntityHelper* entity = entityMan->GetEntity(requset->server_index()))
	{
		if(requset->is_regist())
		{
			if(uint64_t timerId = entity->TimerId())
			{
				entity->TimerId() = 0;
				entityMan->Timer()->killTimer(timerId);
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
