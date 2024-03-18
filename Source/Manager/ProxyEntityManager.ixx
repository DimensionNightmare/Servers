module;
#include "StdAfx.h"

#include <map>
#include <shared_mutex>
export module ProxyEntityManager;

import DNServer;
export import ProxyEntity;
import EntityManager;

using namespace std;

export template<class TEntity = ProxyEntity>
class ProxyEntityManager : public EntityManager<TEntity>
{
public:
    ProxyEntityManager();

	virtual ~ProxyEntityManager();

public: // dll override
	void EntityTimeoutTimer(uint64_t timerID);

protected: // dll proxy
	//
	map<uint64_t, unsigned int > mEntityCloseTimer;

	shared_mutex oMapMutex;
	shared_mutex oMapTimerMutex;
};



template <class TEntity>
ProxyEntityManager<TEntity>::ProxyEntityManager()
{
	mEntityCloseTimer.clear();
}

template <class TEntity>
ProxyEntityManager<TEntity>::~ProxyEntityManager()
{
	mEntityCloseTimer.clear();
}

template <class TEntity>
void ProxyEntityManager<TEntity>::EntityTimeoutTimer(uint64_t timerID)
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
		delete entity;
	}
}
