module;
#include <map>
#include <list>
#include <shared_mutex>
#include <cstdint>

#include "StdAfx.h"
export module ServerEntityManager;

export import ServerEntity;
import EntityManager;

using namespace std;

export class ServerEntityManager : public EntityManager<ServerEntity>
{
public:
    ServerEntityManager();

	virtual ~ServerEntityManager();

	virtual bool Init() override;

public: // dll override
	void EntityCloseTimer(uint64_t timerID);
	
	virtual bool RemoveEntity(uint32_t entityId);

protected: // dll proxy
    map<ServerType, list<ServerEntity*> > mEntityMapList;
	// server pull server
	atomic<uint32_t> iServerId;
	list<uint32_t> mIdleServerId;

};

ServerEntityManager::ServerEntityManager()
{
	mEntityMapList.clear();
	iServerId = 0;
	mIdleServerId.clear();
}

ServerEntityManager::~ServerEntityManager()
{
	mEntityMapList.clear();
}

bool ServerEntityManager::Init()
{
	return EntityManager::Init();
}

void ServerEntityManager::EntityCloseTimer(uint64_t timerID)
{
	unique_lock<shared_mutex> ulock(oTimerMutex);
	if(!mMapTimer.contains(timerID))
	{
		return;
	}

	uint32_t entityId = mMapTimer[timerID];
	if(RemoveEntity(entityId))
	{
		DNPrint(0, LoggerLevel::Debug, "EntityCloseTimer server destory entity\n");
	}
	
}

bool ServerEntityManager::RemoveEntity(uint32_t entityId)
{
	if(mEntityMap.contains(entityId))
	{
		ServerEntity* entity = &mEntityMap[entityId];
		unique_lock<shared_mutex> ulock(oMapMutex);

		mEntityMapList[entity->GetType()].remove(entity);

		if(ServerEntity* owner = entity->LinkNode())
		{
			owner->ClearFlag(ServerEntityFlag::Locked);
		}
		
		mEntityMap.erase(entityId);
		return true;
	}

	return false;
}
