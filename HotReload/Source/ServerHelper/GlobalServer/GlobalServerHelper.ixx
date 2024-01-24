module;
#include "Common.pb.h"
#include "hv/Channel.h"

#include <assert.h>
export module GlobalServerHelper;

import GlobalServer;
import DNClientProxyHelper;
import ServerEntityManagerHelper;
import ServerEntity;

import MessagePack;
import ServerEntityHelper;

using namespace std;
using namespace GMsg::Common;
using namespace hv;

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
	auto manager = GetEntityManager();
	auto gates = manager->GetEntityByList(ServerType::GateServer);
	if(gates.size() <= 0)
		return;

	auto dbs = manager->GetEntityByList(ServerType::DatabaseServer);
	auto logics = manager->GetEntityByList(ServerType::LogicServer);

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
		auto entityHelper = static_cast<ServerEntityHelper*>(it);

		if(!entityHelper->GetChild()->GetSock())
			continue;

		if(gateMap.count(it) && gateMap[it][0] && gateMap[it][1])
		{
			// not exist pos
			manager->UnMountEntity(entityHelper->GetServerType(), it);
		}
		else
		{
			gatesTemp.emplace_back(entityHelper);
		}
	}

	if(gatesTemp.size() <= 0) 
		return;

	
	// alloc gate
	int dbPos = 0;
	int logicPos = 0;
	COM_RetChangeCtlSrv retMsg;
	string binData;

	auto registControl = [&](ServerEntityHelper* beEntity, ServerEntityHelper* entity)
	{
		SocketChannelPtr channel = entity->GetChild()->GetSock();
		entity->SetLinkNode(beEntity);
		entity->GetChild()->SetSock(nullptr);
		channel->setContext(nullptr);
		
		// sendData
		binData.clear();
		retMsg.set_ip(beEntity->GetServerIp());
		retMsg.set_port(beEntity->GetServerPort());
		binData.resize(retMsg.ByteSize());
		retMsg.SerializeToArray(binData.data(), binData.size());
		MessagePack(0, MsgDeal::Req, retMsg.GetDescriptor()->full_name(), binData);
		channel->write(binData);
		
		// timer destory
		auto timerId = GetSSock()->loop(0)->setTimeout(10000, [manager, entity](TimerID timerId)
		{
			manager->RemoveEntity(entity->GetChild()->GetID());
		});

		entity->GetChild()->SetTimerId(timerId);
	};
	
	for(auto it : gatesTemp)
	{
		auto idles = gateMap[it];
		if(!idles[0] && dbPos < dbsTemp.size())
		{
			ServerEntityHelper* entity = dbsTemp[dbPos];
			registControl(it, entity);
			manager->UnMountEntity(entity->GetServerType(), entity);
			dbPos++;
		}

		if(!idles[1] && logicPos < logicsTemp.size())
		{
			ServerEntityHelper* entity = logicsTemp[logicPos];
			registControl(it, entity);
			manager->UnMountEntity(entity->GetServerType(), entity);
			logicPos++;
		}
	}
}
