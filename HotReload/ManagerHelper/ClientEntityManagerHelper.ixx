module;
#include <map>
#include <shared_mutex>
#include "hv/Channel.h"

#include "StdAfx.h"
export module ClientEntityManagerHelper;

import ClientEntityHelper;
import ClientEntityManager;

using namespace std;
using namespace hv;

export template<class TEntity = ClientEntity>
class ClientEntityManagerHelper : public ClientEntityManager<TEntity>
{
private:
	ClientEntityManagerHelper(){}
public:
    ClientEntityHelper* AddEntity(uint32_t entityId);

    void RemoveEntity(uint32_t entityId, bool isDel = true);

    ClientEntityHelper* GetEntity(uint32_t id);
};

template <class TEntity>
ClientEntityHelper* ClientEntityManagerHelper<TEntity>::AddEntity(uint32_t entityId)
{
	ClientEntityHelper* entity = nullptr;

	if (!this->mEntityMap.contains(entityId))
	{
		unique_lock<shared_mutex> ulock(this->oMapMutex);

		TEntity* oriEntity = &this->mEntityMap[entityId];
		
		entity = static_cast<ClientEntityHelper*>(oriEntity);
		entity->ID() = entityId;
	}

	return entity;
}

template <class TEntity>
void ClientEntityManagerHelper<TEntity>::RemoveEntity(uint32_t entityId, bool isDel)
{
	
	if(this->mEntityMap.contains(entityId))
	{
		TEntity* oriEntity = &this->mEntityMap[entityId];
		ClientEntityHelper* entity = static_cast<ClientEntityHelper*>(oriEntity);
		
		unique_lock<shared_mutex> ulock(this->oMapMutex);

		DNPrint(0, LoggerLevel::Debug, "destory client entity\n");
		this->mEntityMap.erase(entityId);
	}
	
}

template <class TEntity>
ClientEntityHelper* ClientEntityManagerHelper<TEntity>::GetEntity(uint32_t entityId)
{
	shared_lock<shared_mutex> lock(this->oMapMutex);
	ClientEntityHelper* entity = nullptr;
	if(this->mEntityMap.contains(entityId))
	{
		TEntity* oriEntity = &this->mEntityMap[entityId];
		return static_cast<ClientEntityHelper*>(oriEntity);
	}
	// allow return empty
	return entity;
}
