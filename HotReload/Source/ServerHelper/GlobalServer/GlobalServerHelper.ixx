module;

#include <assert.h>
export module GlobalServerHelper;

import GlobalServer;
import DNClientProxyHelper;
import ServerEntityManagerHelper;
import ServerEntity;

using namespace std;

export class GlobalServerHelper : public GlobalServer
{
private:
	GlobalServerHelper(){};
public:

	DNClientProxyHelper* GetCSock(){ return nullptr;}
	ServerEntityManagerHelper<ServerEntity>* GetEntityManager(){ return nullptr;}

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
	auto gates = GetEntityManager()->GetEntity(ServerType::GateServer);
	auto dbs = GetEntityManager()->GetEntity(ServerType::DatabaseServer);
	auto logics = GetEntityManager()->GetEntity(ServerType::LogicServer);

	
}
