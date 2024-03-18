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

public: // dll override
	void EntityTimeoutTimer(uint64_t timerID);

protected: // dll proxy
    map<ServerType, list<TEntity*> > mEntityMapList;
	//
	map<uint64_t, unsigned int > mEntityCloseTimer;
	// server pull server
	atomic<unsigned int> iServerId;
	list<unsigned int> mIdleServerId;

	shared_mutex oMapMutex;
	shared_mutex oMapTimerMutex;
};



template <class TEntity>
ServerEntityManager<TEntity>::ServerEntityManager()
{
	mEntityMapList.clear();
	mEntityCloseTimer.clear();
	iServerId = 0;
	mIdleServerId.clear();
}

template <class TEntity>
ServerEntityManager<TEntity>::~ServerEntityManager()
{
	mEntityMapList.clear();
	mEntityCloseTimer.clear();
}

template <class TEntity>
void ServerEntityManager<TEntity>::EntityTimeoutTimer(uint64_t timerID)
{
	unique_lock<shared_mutex> ulock(oMapTimerMutex);
	if(!mEntityCloseTimer.contains(timerID))
	{
		return;
	}

	unsigned int entityId = mEntityCloseTimer[timerID];

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
