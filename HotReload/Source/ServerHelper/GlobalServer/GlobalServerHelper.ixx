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
	if(gates.size() <= 0)
		return;

	auto dbs = GetEntityManager()->GetEntity(ServerType::DatabaseServer);
	auto logics = GetEntityManager()->GetEntity(ServerType::LogicServer);

	vector<ServerEntityHelper*> gatesTemp;
	vector<ServerEntityHelper*> dbsTemp;
	vector<ServerEntityHelper*> logicsTemp;

	map<ServerEntity*, ServerEntityHelper*[2]> gateMap;

	// idle db
	for(auto it : dbs)
	{
		auto entityHelper = static_cast<ServerEntityHelper*>(it);  

		if(!entityHelper->GetLinkNode())
		{
			dbsTemp.emplace_back(entityHelper);
		}
		else
		{
			gateMap[entityHelper->GetLinkNode()][0] = entityHelper;
		}
	}

	// idle logic
	for(auto it : logics)
	{
		auto entityHelper = static_cast<ServerEntityHelper*>(it);

		if(!entityHelper->GetLinkNode())
		{
			logicsTemp.emplace_back(entityHelper);
		}
		else
		{
			gateMap[entityHelper->GetLinkNode()][1] = entityHelper;
		}
	}

	// idle gate
	for(auto it : gates)
	{
		if(gateMap.count(it) && gateMap[it][0] && gateMap[it][1])
		{

		}
		else
		{
			auto entityHelper = static_cast<ServerEntityHelper*>(it);
			gatesTemp.emplace_back(entityHelper);
		}
	}

	if(gatesTemp.size() <= 0) 
		return;

	
	// alloc gate
	int dbPos = 0;
	int logicPos = 0;
	for(auto it : gatesTemp)
	{
		auto idles = gateMap[it];
		if(!idles[0] && dbPos < dbsTemp.size())
		{
			dbsTemp[dbPos]->SetLinkNode(it);
			dbPos++;
		}

		if(!idles[1] && logicPos < logicsTemp.size())
		{
			logicsTemp[logicPos]->SetLinkNode(it);
			logicPos++;
		}
	}
}
