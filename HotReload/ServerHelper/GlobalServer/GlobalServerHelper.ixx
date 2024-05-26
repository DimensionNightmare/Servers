module;
#include <assert.h>
#include <list>
#include "hv/Channel.h"

#include "Server/S_Common.pb.h"
export module GlobalServerHelper;

export import GlobalServer;
export import DNClientProxyHelper;
export import DNServerProxyHelper;
export import ServerEntityManagerHelper;
import MessagePack;

using namespace std;
using namespace hv;
using namespace GMsg;

#define CastObj(entity) static_cast<ServerEntityHelper*>(entity)

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
	assert(PGlobalServerHelper != nullptr);
}

export GlobalServerHelper* GetGlobalServer()
{
	return PGlobalServerHelper;
}


void GlobalServerHelper::UpdateServerGroup()
{
	ServerEntityManagerHelper* entityMan = GetServerEntityManager();

	list<ServerEntity*> gates = entityMan->GetEntityByList(ServerType::GateServer);
	if (gates.size() <= 0)
	{
		return;
	}

	list<ServerEntity*>& dbs = entityMan->GetEntityByList(ServerType::DatabaseServer);
	list<ServerEntity*>& logics = entityMan->GetEntityByList(ServerType::LogicServer);

	// alloc gate
	COM_RetChangeCtlSrv retMsg;
	string binData;

	auto registControl = [&](ServerEntityHelper* beEntity, ServerEntityHelper* entity)
		{
			SocketChannelPtr channel = entity->GetSock();
			entity->LinkNode() = beEntity;
			entity->SetSock(nullptr);
			channel->setContext(nullptr);

			// sendData
			retMsg.set_ip(beEntity->ServerIp());
			retMsg.set_port(beEntity->ServerPort());

			binData.clear();
			binData.resize(retMsg.ByteSizeLong());
			retMsg.SerializeToArray(binData.data(), binData.size());
			MessagePack(0, MsgDeal::Ret, retMsg.GetDescriptor()->full_name().c_str(), binData);
			channel->write(binData);

			// timer destory
			entity->TimerId() = entityMan->ServerEntityManager::CheckEntityCloseTimer(entity->ID());
		};

	for (ServerEntity* it : gates)
	{
		ServerEntityHelper* gate = CastObj(it);
		if (gate->HasFlag(ServerEntityFlag::Locked))
		{
			continue;
		}

		list<ServerEntity*>& gatesDb = gate->GetMapLinkNode(ServerType::DatabaseServer);
		list<ServerEntity*>& gatesLogic = gate->GetMapLinkNode(ServerType::LogicServer);
		if (!dbs.empty() && gatesDb.size() < 1)
		{
			ServerEntity* ele = dbs.front();
			dbs.pop_front();
			ServerEntityHelper* entity = CastObj(ele);
			registControl(gate, entity);
			entityMan->UnMountEntity(entity->ServerEntityType(), ele);
			gatesDb.emplace_back(ele);
		}

		if (!logics.empty() && gatesLogic.size() < 1)
		{
			ServerEntity* ele = logics.front();
			logics.pop_front();
			ServerEntityHelper* entity = CastObj(ele);
			registControl(gate, entity);
			entityMan->UnMountEntity(entity->ServerEntityType(), ele);
			gatesLogic.emplace_back(ele);
		}

		if (gatesDb.size() && gatesLogic.size())
		{
			// UnMountEntity(gate->GetServerType(), it);
			gate->SetFlag(ServerEntityFlag::Locked);
		}

	}
}
