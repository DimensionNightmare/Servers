module;
#include <map>
#include <shared_mutex>
#include <cstdint>
#include "hv/Channel.h"

#include "StdAfx.h"
export module ClientEntityManagerHelper;

export import ClientEntityHelper;
export import ClientEntityManager;

using namespace std;
using namespace hv;

export class ClientEntityManagerHelper : public ClientEntityManager
{
private:
	ClientEntityManagerHelper(){}
public:
    ClientEntityHelper* AddEntity(uint32_t entityId);

    virtual bool RemoveEntity(uint32_t entityId);

    ClientEntityHelper* GetEntity(uint32_t id);
};

ClientEntityHelper* ClientEntityManagerHelper::AddEntity(uint32_t entityId)
{
	ClientEntityHelper* entity = nullptr;

	if (!mEntityMap.contains(entityId))
	{
		unique_lock<shared_mutex> ulock(oMapMutex);

		ClientEntity* oriEntity = &mEntityMap[entityId];
		
		entity = static_cast<ClientEntityHelper*>(oriEntity);
		entity->ID() = entityId;
	}

	return entity;
}

bool ClientEntityManagerHelper::RemoveEntity(uint32_t entityId)
{
	
	if(mEntityMap.contains(entityId))
	{	
		unique_lock<shared_mutex> ulock(oMapMutex);

		DNPrint(0, LoggerLevel::Debug, "destory client entity");
		mEntityMap.erase(entityId);

		return true;
	}
	
	return false;
}


ClientEntityHelper* ClientEntityManagerHelper::GetEntity(uint32_t entityId)
{
	shared_lock<shared_mutex> lock(oMapMutex);
	ClientEntityHelper* entity = nullptr;
	if(mEntityMap.contains(entityId))
	{
		ClientEntity* oriEntity = &mEntityMap[entityId];
		return static_cast<ClientEntityHelper*>(oriEntity);
	}
	// allow return empty
	return entity;
}
