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

    virtual bool RemoveEntity(uint32_t entityId);

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
bool ProxyEntityManagerHelper<TEntity>::RemoveEntity(uint32_t entityId)
{
	if(this->mEntityMap.contains(entityId))
	{
		TEntity* oriEntity = &this->mEntityMap[entityId];
		ProxyEntityHelper* entity = static_cast<ProxyEntityHelper*>(oriEntity);
		
		unique_lock<shared_mutex> ulock(this->oMapMutex);
		
		DNPrint(0, LoggerLevel::Debug, "destory Proxy entity\n");
		this->mEntityMap.erase(entityId);
		return true;
	}
	
	return false;
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
