module;
#include "StdMacro.h"
export module GlobalServerHelper;

import GlobalServer;
import DNClientProxyHelper;
import DNServerProxyHelper;
import ServerEntityManagerHelper;
import FuncHelper;
import Macro;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;

export class GlobalServerHelper : public GlobalServer
{

private:

	GlobalServerHelper() = delete;;
public:

	DNClientProxyHelper* GetCSock() { return nullptr; }

	DNServerProxyHelper* GetSSock() { return nullptr; }

	ServerEntityManagerHelper* GetServerEntityManager() { return nullptr; }

	void UpdateServerGroup()
	{
		ServerEntityManagerHelper* entityMan = GetServerEntityManager();

		list<ServerEntity*> gates = entityMan->GetEntitysByType(EMServerType::GateServer);
		if (gates.empty())
		{
			return;
		}

		list<ServerEntity*> dbs = entityMan->GetEntitysByType(EMServerType::DatabaseServer);
		list<ServerEntity*> logics = entityMan->GetEntitysByType(EMServerType::LogicServer);

		// alloc gate
		COM_RetChangeCtlSrv request;
		string binData;

		auto registControl = [&](ServerEntity* beEntity, ServerEntity* entity)
		{
			const SocketChannelPtr& channel = entity->GetSock();
			entity->LinkNode() = beEntity;

			channel->setContext(nullptr);

			// sendData
			request.set_server_ip(beEntity->ServerIp());
			request.set_server_port(beEntity->ServerPort());

			request.SerializeToString(&binData);
			// timer destory
			entity->TimerId() = TICK_MAINSPACE_SIGN_FUNCTION(ServerEntityManager, CheckEntityCloseTimer, entityMan, entity->ID());
			MessagePackAndSend(0, EMMsgDeal::Ret, request.GetDescriptor()->full_name().c_str(), binData, channel);
			entity->SetSock(nullptr);
		};

		for (ServerEntity* gate : gates)
		{
			if (gate->HasFlag(EMServerEntityFlag::Locked))
			{
				continue;
			}

			list<ServerEntity*>& gatesDb = gate->GetMapLinkNode(EMServerType::DatabaseServer);
			list<ServerEntity*>& gatesLogic = gate->GetMapLinkNode(EMServerType::LogicServer);
			if (!dbs.empty() && gatesDb.size() < 1)
			{
				ServerEntity* ele = dbs.front();
				// dbs.pop_front();
				registControl(gate, ele);
				entityMan->UnMountEntity(ele->GetServerType(), ele);
				gatesDb.emplace_back(ele);
			}

			if (!logics.empty() && gatesLogic.size() < 1)
			{
				ServerEntity* ele = logics.front();
				// logics.pop_front();
				registControl(gate, ele);
				entityMan->UnMountEntity(ele->GetServerType(), ele);
				gatesLogic.emplace_back(ele);
			}

			if (gatesDb.size() && gatesLogic.size())
			{
				// UnMountEntity(gate->GetServerType(), it);
				gate->SetFlag(EMServerEntityFlag::Locked);
				DNPrint(0, EMLoggerLevel::Debug, "Gate:%u locked!", gate->ID());
			}

		}
	}
};

static GlobalServerHelper* PGlobalServerHelper = nullptr;

export void SetGlobalServer(GlobalServer* server)
{
	PGlobalServerHelper = static_cast<GlobalServerHelper*>(server);
	ASSERT(PGlobalServerHelper != nullptr)
}

export GlobalServerHelper* GetGlobalServer()
{
	return PGlobalServerHelper;
}
