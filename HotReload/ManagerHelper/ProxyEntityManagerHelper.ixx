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

export class ProxyEntityManagerHelper : public ProxyEntityManager
{
private:
	ProxyEntityManagerHelper(){}
public:
    ProxyEntityHelper* AddEntity(uint32_t entityId);

    virtual bool RemoveEntity(uint32_t entityId);

    ProxyEntityHelper* GetEntity(uint32_t id);
};

ProxyEntityHelper* ProxyEntityManagerHelper::AddEntity(uint32_t entityId)
{
	ProxyEntityHelper* entity = nullptr;

	if (!mEntityMap.contains(entityId))
	{
		unique_lock<shared_mutex> ulock(oMapMutex);

		ProxyEntity* oriEntity = &mEntityMap[entityId];
		
		entity = static_cast<ProxyEntityHelper*>(oriEntity);
		entity->ID() = entityId;
	}

	return entity;
}

bool ProxyEntityManagerHelper::RemoveEntity(uint32_t entityId)
{
	if(mEntityMap.contains(entityId))
	{
		ProxyEntity* oriEntity = &mEntityMap[entityId];
		ProxyEntityHelper* entity = static_cast<ProxyEntityHelper*>(oriEntity);
		
		unique_lock<shared_mutex> ulock(oMapMutex);
		
		DNPrint(0, LoggerLevel::Debug, "destory Proxy entity\n");
		mEntityMap.erase(entityId);
		return true;
	}
	
	return false;
}

ProxyEntityHelper* ProxyEntityManagerHelper::GetEntity(uint32_t entityId)
{
	shared_lock<shared_mutex> lock(oMapMutex);
	ProxyEntityHelper* entity = nullptr;
	if(mEntityMap.contains(entityId))
	{
		ProxyEntity* oriEntity = &mEntityMap[entityId];
		return static_cast<ProxyEntityHelper*>(oriEntity);
	}
	// allow return empty
	return entity;
}
