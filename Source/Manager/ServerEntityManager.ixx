module;
#include <map>
#include <list>
#include <shared_mutex>

#include "StdAfx.h"
export module ServerEntityManager;

import DNServer;
export import ServerEntity;
import EntityManager;

using namespace std;

export template<class TEntity = ServerEntity>
class ServerEntityManager : public EntityManager<ServerEntity>
{
public:
    ServerEntityManager();

	virtual ~ServerEntityManager();

	virtual bool Init() override;

public: // dll override
	void EntityCloseTimer(uint64_t timerID);
	
	virtual bool RemoveEntity(uint32_t entityId);

protected: // dll proxy
    map<ServerType, list<TEntity*> > mEntityMapList;
	// server pull server
	atomic<uint32_t> iServerId;
	list<uint32_t> mIdleServerId;

};



template <class TEntity>
ServerEntityManager<TEntity>::ServerEntityManager()
{
	mEntityMapList.clear();
	iServerId = 0;
	mIdleServerId.clear();
}

template <class TEntity>
ServerEntityManager<TEntity>::~ServerEntityManager()
{
	mEntityMapList.clear();
}

template <class TEntity>
bool ServerEntityManager<TEntity>::Init()
{
	return EntityManager<TEntity>::Init();
}

template <class TEntity>
void ServerEntityManager<TEntity>::EntityCloseTimer(uint64_t timerID)
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

template <class TEntity>
bool ServerEntityManager<TEntity>::RemoveEntity(uint32_t entityId)
{
	if(this->mEntityMap.contains(entityId))
	{
		TEntity* entity = &this->mEntityMap[entityId];
		unique_lock<shared_mutex> ulock(this->oMapMutex);

		this->mEntityMapList[entity->GetType()].remove(entity);

		if(TEntity* owner = entity->LinkNode())
		{
			owner->ClearFlag(ServerEntityFlag::Locked);
		}
		
		this->mEntityMap.erase(entityId);
		return true;
	}

	return false;
}
