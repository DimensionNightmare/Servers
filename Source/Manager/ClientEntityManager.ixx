module;
#include <map>
#include <list>
#include <shared_mutex>

#include "StdAfx.h"
export module ClientEntityManager;

import DNServer;
export import ClientEntity;
import EntityManager;

using namespace std;

export class ClientEntityManager : public EntityManager<ClientEntity>
{
public:
    ClientEntityManager(){};

	virtual ~ClientEntityManager(){};

	virtual bool Init() override;

public: // dll override
	virtual bool RemoveEntity(uint32_t entityId);

protected: // dll proxy
    

};

bool ClientEntityManager::Init()
{
	return EntityManager::Init();
}

bool ClientEntityManager::RemoveEntity(uint32_t entityId)
{
	if(mEntityMap.contains(entityId))
	{
		ClientEntity* entity = &mEntityMap[entityId];
		unique_lock<shared_mutex> ulock(oMapMutex);
		
		mEntityMap.erase(entityId);
		return true;
	}

	return false;
}