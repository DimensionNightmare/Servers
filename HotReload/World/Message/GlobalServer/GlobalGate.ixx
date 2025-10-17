export module GlobalServerMessage:GlobalGate;

import GlobalServerHelper;
import std;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import Logger;

export namespace GlobalServerMessage
{

	void Exe_RetRegistSrv(SocketChannel::CVPtr channel, const std::string& binMsg)
	{
		GMsg::g2G_RetRegistSrv request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}

		GlobalServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<GlobalServerHelper>(EMSystemType::Server);
		ServerEntityManagerHelper::Ptr entityMan = dnServer->GetServerEntityManager();
		if (ServerEntityHelper::CVPtr entity = entityMan->GetEntity(request.serverid()))
		{
			if (request.isregist())
			{
				if (uint64_t timerId = entity->TimerId())
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

				entityMan->RemoveEntity(request.serverid());
				dnServer->UpdateServerGroup();
			}
		}
	}

	void Exe_RetRegistChild(SocketChannel::CVPtr channel, const std::string& binMsg)
	{
		GMsg::g2G_RetRegistChild request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}

		GlobalServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<GlobalServerHelper>(EMSystemType::Server);
		ServerEntityManagerHelper::Ptr entityMan = dnServer->GetServerEntityManager();

		ServerEntityHelper::Ptr entity = entityMan->GetEntity(request.serverid());
		if(!entity)
		{
			return;
		}

		for (int i = 0; i < request.childs_size(); i++)
		{
			const GMsg::COM_ReqRegistSrv& child = request.childs(i);
			EMServerType childType = (EMServerType)child.servertype();
			ServerEntityHelper::Ptr servChild = entityMan->AddEntity(child.serverid(), childType);
			entity->SetMapLinkNode(childType, servChild->GetSelf<ServerEntity>());
		}
	}

}
