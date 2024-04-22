module;
#include <map>
#include <shared_mutex>
#include "hv/Channel.h"

#include "StdAfx.h"
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
    ProxyEntityHelper* AddEntity(uint32_t entityId);

    void RemoveEntity(uint32_t entityId, bool isDel = true);

    ProxyEntityHelper* GetEntity(uint32_t id);
};

template <class TEntity>
ProxyEntityHelper* ProxyEntityManagerHelper<TEntity>::AddEntity(uint32_t entityId)
{
	ProxyEntityHelper* entity = nullptr;

	if (!this->mEntityMap.contains(entityId))
	{
		unique_lock<shared_mutex> ulock(this->oMapMutex);

		TEntity* oriEntity = &this->mEntityMap[entityId];
		
		entity = static_cast<ProxyEntityHelper*>(oriEntity);
		entity->ID() = entityId;
	}

	return entity;
}

template <class TEntity>
void ProxyEntityManagerHelper<TEntity>::RemoveEntity(uint32_t entityId, bool isDel)
{
	
	if(this->mEntityMap.contains(entityId))
	{
		TEntity* oriEntity = &this->mEntityMap[entityId];
		ProxyEntityHelper* entity = static_cast<ProxyEntityHelper*>(oriEntity);
		if(isDel)
		{
			unique_lock<shared_mutex> ulock(this->oMapMutex);

			// this->mIdleServerId.push_back(entityId);
			
			DNPrint(0, LoggerLevel::Debug, "destory entity\n");
			this->mEntityMap.erase(entityId);
		}
		else
		{
			entity->SetSock(nullptr);
		}
	}
	
}

template <class TEntity>
ProxyEntityHelper* ProxyEntityManagerHelper<TEntity>::GetEntity(uint32_t entityId)
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
