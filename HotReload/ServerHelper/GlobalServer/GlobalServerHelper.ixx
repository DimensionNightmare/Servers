module;
#include <list>
#include <string>

#include "StdMacro.h"
export module GlobalServerHelper;

export import GlobalServer;
export import DNClientProxyHelper;
export import DNServerProxyHelper;
export import ServerEntityManagerHelper;
import MessagePack;
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

	void UpdateServerGroup();
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


void GlobalServerHelper::UpdateServerGroup()
{
	ServerEntityManagerHelper* entityMan = GetServerEntityManager();

	list<ServerEntity*> gates = entityMan->GetEntityByList(ServerType::GateServer);
	if (gates.empty())
	{
		return;
	}

	list<ServerEntity*> dbs = entityMan->GetEntityByList(ServerType::DatabaseServer);
	list<ServerEntity*> logics = entityMan->GetEntityByList(ServerType::LogicServer);

	// alloc gate
	COM_RetChangeCtlSrv request;
	string binData;

	auto registControl = [&](ServerEntity* beEntity, ServerEntity* entity)
		{
			const SocketChannelPtr& channel = entity->GetSock();
			entity->LinkNode() = beEntity;

			channel->setContext(nullptr);

			// sendData
			request.set_ip(beEntity->ServerIp());
			request.set_port(beEntity->ServerPort());

			request.SerializeToString(&binData);
			MessagePack(0, MsgDeal::Ret, request.GetDescriptor()->full_name().c_str(), binData);
			channel->write(binData);

			// timer destory
			entity->TimerId() = TICK_MAINSPACE_SIGN_FUNCTION(ServerEntityManager, CheckEntityCloseTimer, entityMan, entity->ID());

			entity->SetSock(nullptr);
		};

	for (ServerEntity* gate : gates)
	{
		if (gate->HasFlag(ServerEntityFlag::Locked))
		{
			continue;
		}

		list<ServerEntity*>& gatesDb = gate->GetMapLinkNode(ServerType::DatabaseServer);
		list<ServerEntity*>& gatesLogic = gate->GetMapLinkNode(ServerType::LogicServer);
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
			gate->SetFlag(ServerEntityFlag::Locked);
			DNPrint(0, LoggerLevel::Debug, "Gate:%u locked!", gate->ID());
		}

	}
}
