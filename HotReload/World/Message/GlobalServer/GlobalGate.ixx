export module GlobalServerMessage:GlobalGate;

import GlobalServerHelper;
import ThirdParty.PbGen;
import Logger;
import GlobalServerMessage;

namespace MsgHandleRegister
{

	HandleRegistry<GMsg::g2G_RetRegistSrv, void, EMMsgDeal::Ret> Exe_RetRegistSrv =
				[](auto request, SocketChannel::Ptr channel)
	{
		GlobalServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<GlobalServerHelper>(EMSystemType::Server);
		ServerEntityManagerHelper::Ptr entityMan = dnServer->GetServerEntityManager();
		if (ServerEntityHelper::Ptr entity = entityMan->GetEntity(request->serverid()))
		{
			if (request->isregist())
			{
				if (size_t timerId = entity->GetTimerId())
				{
					entity->SetTimerId(0);
					entityMan->GetTimer()->KillTimer(timerId);
				}
			}
			else
			{
				ServerEntity::Ptr owner = channel->getContextPtr<ServerEntity>();
				// remove and unlock
				owner->GetMapLinkNode(entity->GetServerType()).remove(entity);
				owner->ClearFlag(EMServerEntityFlag::Locked);

				LoggerPrint::Log(channel, ELogLevel_Debug, "Global get notify release gate lock!");

				entityMan->RemoveEntity(request->serverid());
				dnServer->UpdateServerGroup();
			}
		}
	};

	HandleRegistry<GMsg::g2G_RetRegistChild, void, EMMsgDeal::Ret> Exe_RetRegistChild =
				[](auto request, SocketChannel::Ptr channel)
	{
	
		GlobalServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<GlobalServerHelper>(EMSystemType::Server);
		ServerEntityManagerHelper::Ptr entityMan = dnServer->GetServerEntityManager();

		ServerEntityHelper::Ptr entity = entityMan->GetEntity(request->serverid());
		if(!entity)
		{
			return;
		}

		for (int i = 0; i < request->childs_size(); i++)
		{
			const GMsg::COM_ReqRegistSrv& child = request->childs(i);
			EMServerType childType = (EMServerType)child.servertype();
			ServerEntityHelper::Ptr servChild = entityMan->AddEntity(child.serverid(), childType);
			entity->SetMapLinkNode(childType, servChild->GetSelf<ServerEntity>());
		}
	};
}
