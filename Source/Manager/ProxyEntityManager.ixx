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

	virtual bool Init() override;

public: // dll override
	void EntityCloseTimer(uint64_t timerID);

protected: // dll proxy

	
};



template <class TEntity>
ProxyEntityManager<TEntity>::ProxyEntityManager()
{
}

template <class TEntity>
ProxyEntityManager<TEntity>::~ProxyEntityManager()
{
}

template <class TEntity>
bool ProxyEntityManager<TEntity>::Init()
{
	return EntityManager<TEntity>::Init();
}

template <class TEntity>
void ProxyEntityManager<TEntity>::EntityCloseTimer(uint64_t timerID)
{
	unique_lock<shared_mutex> ulock(this->oTimerMutex);
	if(!this->mMapTimer.contains(timerID))
	{
		return;
	}

	unsigned int entityId = this->mMapTimer[timerID];

	if(this->mEntityMap.contains(entityId))
	{
		TEntity* entity = &this->mEntityMap[entityId];
		unique_lock<shared_mutex> ulock(this->oMapMutex);

		DNPrint(-1, LoggerLevel::Debug, "destory proxy entity\n");
		this->mEntityMap.erase(entityId);
	}
}
