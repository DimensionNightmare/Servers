module;
#include <map>
#include <shared_mutex>
#include <cstdint>
#include <list>
#include "hv/Channel.h"

#include "StdAfx.h"
export module ServerEntityManagerHelper;

export import ServerEntityManager;
export import ServerEntityHelper;

using namespace std;
using namespace hv;

export class ServerEntityManagerHelper : public ServerEntityManager
{
private:
	ServerEntityManagerHelper(){}
public:
    ServerEntityHelper* AddEntity(uint32_t entityId, ServerType type);

    virtual bool RemoveEntity(uint32_t entityId);

	void MountEntity(ServerType type, ServerEntity* entity);

    void UnMountEntity(ServerType type, ServerEntity* entity);

    ServerEntityHelper* GetEntity(uint32_t id);

	list<ServerEntity*>& GetEntityByList(ServerType type);

	[[nodiscard]] uint32_t ServerIndex();
};

ServerEntityHelper* ServerEntityManagerHelper::AddEntity(uint32_t entityId, ServerType regType)
{
	ServerEntityHelper* entity = nullptr;

	if (!mEntityMap.contains(entityId))
	{
		unique_lock<shared_mutex> ulock(oMapMutex);

		ServerEntity* oriEntity = &mEntityMap[entityId];
		mEntityMapList[regType].emplace_back(oriEntity);
		
		entity = static_cast<ServerEntityHelper*>(oriEntity);
		entity->ID() = entityId;
		entity->ServerEntityType() = regType;
	}

	return entity;
}

bool ServerEntityManagerHelper::RemoveEntity(uint32_t entityId)
{
	if(mEntityMap.contains(entityId))
	{
		ServerEntity* oriEntity = &mEntityMap[entityId];
		ServerEntityHelper* entity = static_cast<ServerEntityHelper*>(oriEntity);
		
		unique_lock<shared_mutex> ulock(oMapMutex);

		mEntityMapList[entity->ServerEntityType()].remove(oriEntity);
		
		DNPrint(0, LoggerLevel::Debug, "offline destory entity");
		mEntityMap.erase(entityId);
		return true;
	}
	
	return false;
}

void ServerEntityManagerHelper::MountEntity(ServerType type, ServerEntity *entity)
{
	unique_lock<shared_mutex> ulock(oMapMutex);
	// mEntityMap mEntityMapList
	mEntityMapList[type].push_back(entity);
}

void ServerEntityManagerHelper::UnMountEntity(ServerType type, ServerEntity *entity)
{
	unique_lock<shared_mutex> ulock(oMapMutex);
	// mEntityMap mEntityMapList
	mEntityMapList[type].remove(entity);
}

ServerEntityHelper* ServerEntityManagerHelper::GetEntity(uint32_t entityId)
{
	shared_lock<shared_mutex> lock(oMapMutex);
	ServerEntityHelper* entity = nullptr;
	if(mEntityMap.contains(entityId))
	{
		ServerEntity* oriEntity = &mEntityMap[entityId];
		return static_cast<ServerEntityHelper*>(oriEntity);
	}
	// allow return empty
	return entity;
}

list<ServerEntity*>& ServerEntityManagerHelper::GetEntityByList(ServerType type)
{
	shared_lock<shared_mutex> lock(oMapMutex);
	return mEntityMapList[type];
}

uint32_t ServerEntityManagerHelper::ServerIndex()
{
	// if(mIdleServerId.size() > 0)
	// {
	// 	uint32_t index = mIdleServerId.front();
	// 	mIdleServerId.pop_front();
	// 	return index;
	// }
	return ++iServerId;
}
