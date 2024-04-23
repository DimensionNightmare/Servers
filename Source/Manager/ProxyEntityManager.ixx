module;
#include <map>
#include <shared_mutex>

#include "StdAfx.h"
export module ProxyEntityManager;

import DNServer;
export import ProxyEntity;
import EntityManager;

using namespace std;

export class ProxyEntityManager : public EntityManager<ProxyEntity>
{
public:
    ProxyEntityManager(){};

	virtual ~ProxyEntityManager(){};

	virtual bool Init() override;

public: // dll override
	void EntityCloseTimer(uint64_t timerID);

	virtual bool RemoveEntity(uint32_t entityId);

protected: // dll proxy

	
};

bool ProxyEntityManager::Init()
{
	return EntityManager::Init();
}

void ProxyEntityManager::EntityCloseTimer(uint64_t timerID)
{
	unique_lock<shared_mutex> ulock(oTimerMutex);
	if(!mMapTimer.contains(timerID))
	{
		return;
	}

	uint32_t entityId = mMapTimer[timerID];
	if(RemoveEntity(entityId))
	{
		DNPrint(0, LoggerLevel::Debug, "destory proxy Timer entity\n");
	}
	
}

bool ProxyEntityManager::RemoveEntity(uint32_t entityId)
{
	if(mEntityMap.contains(entityId))
	{
		ProxyEntity* entity = &mEntityMap[entityId];
		unique_lock<shared_mutex> ulock(oMapMutex);

		mEntityMap.erase(entityId);
		return true;
	}

	return false;
}