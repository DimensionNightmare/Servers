module;
#include "StdMacro.h"
export module GlobalMessage:GlobalGate;

import DNTask;
import FuncHelper;
import GlobalServerHelper;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import Logger;
import DNClientProxyHelper;
import ServerEntityManagerHelper;

namespace GlobalMessage
{

	export void Exe_RetRegistSrv(SocketChannelPtr channel, string binMsg)
	{
		g2G_RetRegistSrv request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}

		GlobalServerHelper* dnServer = GetGlobalServer();
		ServerEntityManagerHelper* entityMan = dnServer->GetServerEntityManager();
		if (ServerEntity* entity = entityMan->GetEntity(request.server_id()))
		{
			if (request.is_regist())
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

				DNPrint(0, LoggerLevel::Debug, "Global get notify release gate lock!");

				entityMan->RemoveEntity(request.server_id());
				dnServer->UpdateServerGroup();
			}
		}
	}

	export void Exe_RetRegistChild(SocketChannelPtr channel, string binMsg)
	{
		g2G_RetRegistChild request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}

		GlobalServerHelper* dnServer = GetGlobalServer();
		ServerEntityManagerHelper* entityMan = dnServer->GetServerEntityManager();

		ServerEntity* entity = entityMan->GetEntity(request.server_id());

		for (int i = 0; i < request.childs_size(); i++)
		{
			const COM_ReqRegistSrv& child = request.childs(i);
			ServerType childType = (ServerType)child.server_type();
			ServerEntity* servChild = entityMan->AddEntity(child.server_id(), childType);
			entity->SetMapLinkNode(childType, servChild);
		}
	}
}
