module;
#include "StdAfx.h"

#include "hv/Channel.h"
#include <map>
#include <shared_mutex>
export module ProxyEntityManagerHelper;

import ProxyEntityHelper;
import ProxyEntityManager;

using namespace std;
using namespace hv;

export template<class TEntity = ProxyEntity>
class ProxyEntityManagerHelper : public ProxyEntityManager<TEntity>
{
private:
	ProxyEntityManagerHelper(){}
public:
    ProxyEntityHelper* AddEntity(unsigned int entityId);

    void RemoveEntity(unsigned int entityId, bool isDel = true);

    ProxyEntityHelper* GetEntity(unsigned int id);
};

template <class TEntity>
ProxyEntityHelper* ProxyEntityManagerHelper<TEntity>::AddEntity(unsigned int entityId)
{
	ProxyEntityHelper* entity = nullptr;

	if (!this->mEntityMap.contains(entityId))
	{
		unique_lock<shared_mutex> ulock(this->oMapMutex);

		TEntity* oriEntity = &this->mEntityMap[entityId];
		
		entity = static_cast<ProxyEntityHelper*>(oriEntity);
		entity->GetChild()->ID() = entityId;
	}

	return entity;
}

template <class TEntity>
void ProxyEntityManagerHelper<TEntity>::RemoveEntity(unsigned int entityId, bool isDel)
{
	
	if(this->mEntityMap.contains(entityId))
	{
		TEntity* oriEntity = &this->mEntityMap[entityId];
		ProxyEntityHelper* entity = static_cast<ProxyEntityHelper*>(oriEntity);
		if(isDel)
		{
			unique_lock<shared_mutex> ulock(this->oMapMutex);

			// this->mIdleServerId.push_back(entityId);
			
			DNPrint(-1, LoggerLevel::Debug, "destory entity\n");
			this->mEntityMap.erase(entityId);
		}
		else
		{
			entity->GetChild()->SetSock(nullptr);
		}
	}
	
}

template <class TEntity>
ProxyEntityHelper* ProxyEntityManagerHelper<TEntity>::GetEntity(unsigned int entityId)
{
	shared_lock<shared_mutex> lock(this->oMapMutex);
	ProxyEntityHelper* entity = nullptr;
	if(this->mEntityMap.contains(entityId))
	{
		TEntity* oriEntity = &this->mEntityMap[entityId];
		return static_cast<ProxyEntityHelper*>(oriEntity);
	}
	// allow return empty
	return entity;
}
