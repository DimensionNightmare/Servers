module;
#include <coroutine>
#include <cstdint>
#include "hv/Channel.h"

#include "Server/S_Global.pb.h"
export module GlobalMessage:GlobalGate;

import DNTask;
import MessagePack;
import GlobalServerHelper;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg;

namespace GlobalMessage
{

	export void Exe_RetRegistSrv(SocketChannelPtr channel, Message* msg)
	{
		g2G_RetRegistSrv* requset = reinterpret_cast<g2G_RetRegistSrv*>(msg);

		GlobalServerHelper* dnServer = GetGlobalServer();
		ServerEntityManagerHelper* entityMan = dnServer->GetServerEntityManager();
		if (ServerEntity* entity = entityMan->GetEntity(requset->server_index()))
		{
			if (requset->is_regist())
			{
				if (uint64_t timerId = entity->TimerId())
				{
					entity->TimerId() = 0;
					entityMan->Timer()->killTimer(timerId);
				}
			}
			else
			{
				ServerEntity* owner = channel->getContext<ServerEntity>();
				// remove and unlock
				owner->GetMapLinkNode(entity->GetServerType()).remove(entity);
				owner->ClearFlag(ServerEntityFlag::Locked);

				entityMan->RemoveEntity(requset->server_index());
				dnServer->UpdateServerGroup();
			}
		}
	}

}
