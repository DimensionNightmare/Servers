module;
#include <coroutine>
#include <cstdint>

export module GlobalMessage:GlobalGate;

import DNTask;
import MessagePack;
import GlobalServerHelper;
import ThirdParty.Libhv;
import ThirdParty.PbGen;

namespace GlobalMessage
{

	export void Exe_RetRegistSrv(SocketChannelPtr channel, Message* msg)
	{
		g2G_RetRegistSrv* request = reinterpret_cast<g2G_RetRegistSrv*>(msg);

		GlobalServerHelper* dnServer = GetGlobalServer();
		ServerEntityManagerHelper* entityMan = dnServer->GetServerEntityManager();
		if (ServerEntity* entity = entityMan->GetEntity(request->server_index()))
		{
			if (request->is_regist())
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

				entityMan->RemoveEntity(request->server_index());
				dnServer->UpdateServerGroup();
			}
		}
	}

}
