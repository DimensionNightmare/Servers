module;
#include "StdAfx.h"

#include "hv/Channel.h"
#include <map>
#include <shared_mutex>
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
    ClientEntityHelper* AddEntity(unsigned int entityId);

    void RemoveEntity(unsigned int entityId, bool isDel = true);

    ClientEntityHelper* GetEntity(unsigned int id);
};

template <class TEntity>
ClientEntityHelper* ClientEntityManagerHelper<TEntity>::AddEntity(unsigned int entityId)
{
	ClientEntityHelper* entity = nullptr;

	if (!this->mEntityMap.contains(entityId))
	{
		unique_lock<shared_mutex> ulock(this->oMapMutex);

		TEntity* oriEntity = &this->mEntityMap[entityId];
		
		entity = static_cast<ClientEntityHelper*>(oriEntity);
		entity->GetChild()->ID() = entityId;
	}

	return entity;
}

template <class TEntity>
void ClientEntityManagerHelper<TEntity>::RemoveEntity(unsigned int entityId, bool isDel)
{
	
	if(this->mEntityMap.contains(entityId))
	{
		TEntity* oriEntity = &this->mEntityMap[entityId];
		ClientEntityHelper* entity = static_cast<ClientEntityHelper*>(oriEntity);
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
ClientEntityHelper* ClientEntityManagerHelper<TEntity>::GetEntity(unsigned int entityId)
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
