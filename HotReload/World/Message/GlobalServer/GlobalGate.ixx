export module GlobalServerMessage:GlobalGate;

import GlobalServerHelper;
import std;
import ThirdParty.Libhv;

namespace GlobalServerMessage
{

	export void Exe_RetRegistSrv(SocketChannel::CVPtr channel, const std::string& binMsg)
	{
		GMsg::g2G_RetRegistSrv request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}

		GlobalServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<GlobalServerHelper>(EMSystemType::Server);
		ServerEntityManagerHelper::Ptr entityMan = dnServer->GetServerEntityManager();
		if (ServerEntityHelper::CVPtr entity = entityMan->GetEntity(request.server_id()))
		{
			if (request.is_regist())
			{
				if (uint64_t timerId = entity->TimerId())
				{
					entity->SetTimerId(0);
					entityMan->RemoveTimerRecord(timerId);
				}
			}
			else
			{
				ServerEntity::Ptr owner = channel->getContextPtr<ServerEntity>();
				// remove and unlock
				owner->GetMapLinkNode(entity->GetServerType()).remove(entity);
				owner->ClearFlag(EMServerEntityFlag::Locked);

				dnServer->GetLogger()->Record(ELogLevel_Debug, "Global get notify release gate lock!");

				entityMan->RemoveEntity(request.server_id());
				dnServer->UpdateServerGroup();
			}
		}
	}

	export void Exe_RetRegistChild(SocketChannel::CVPtr channel, const std::string& binMsg)
	{
		GMsg::g2G_RetRegistChild request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}

		GlobalServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<GlobalServerHelper>(EMSystemType::Server);
		ServerEntityManagerHelper::Ptr entityMan = dnServer->GetServerEntityManager();

		ServerEntityHelper::Ptr entity = entityMan->GetEntity(request.server_id());
		if(!entity)
		{
			return;
		}

		for (int i = 0; i < request.childs_size(); i++)
		{
			const GMsg::COM_ReqRegistSrv& child = request.childs(i);
			EMServerType childType = (EMServerType)child.server_type();
			ServerEntityHelper::Ptr servChild = entityMan->AddEntity(child.server_id(), childType);
			entity->SetMapLinkNode(childType, servChild->GetSelf<ServerEntity>());
		}
	}
}
