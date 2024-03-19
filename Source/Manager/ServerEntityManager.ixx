module;
#include "StdAfx.h"

#include <map>
#include <list>
#include <shared_mutex>
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

protected: // dll proxy
    map<ServerType, list<TEntity*> > mEntityMapList;
	// server pull server
	atomic<unsigned int> iServerId;
	list<unsigned int> mIdleServerId;

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

	unsigned int entityId = mMapTimer[timerID];

	if(this->mEntityMap.contains(entityId))
	{
		TEntity* entity = this->mEntityMap[entityId];
		unique_lock<shared_mutex> ulock(this->oMapMutex);

		DNPrint(-1, LoggerLevel::Debug, "destory entity\n");
		this->mEntityMap.erase(entityId);
		this->mEntityMapList[entity->GetType()].remove(entity);
		delete entity;
	}
}
