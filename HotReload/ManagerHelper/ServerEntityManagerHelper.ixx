module;
#include <shared_mutex>
#include <cstdint>
#include <list>

#include "StdMacro.h"
export module ServerEntityManagerHelper;

export import ServerEntityManager;
export import ServerEntityHelper;
import Logger;

using namespace std;

export class ServerEntityManagerHelper : public ServerEntityManager
{
private:
	ServerEntityManagerHelper() {}
public:
	ServerEntity* AddEntity(uint32_t entityId, ServerType type);

	bool RemoveEntity(uint32_t entityId);

	void MountEntity(ServerType type, ServerEntity* entity);

	void UnMountEntity(ServerType type, ServerEntity* entity);

	ServerEntity* GetEntity(uint32_t id);

	const list<ServerEntity*>& GetEntityByList(ServerType type);

	[[nodiscard]] uint32_t ServerIndex();
};

ServerEntity* ServerEntityManagerHelper::AddEntity(uint32_t entityId, ServerType regType)
{
	if (!mEntityMap.contains(entityId))
	{
		unique_lock<shared_mutex> ulock(oMapMutex);

		mEntityMap.emplace(std::piecewise_construct,
			std::forward_as_tuple(entityId),
			std::forward_as_tuple(entityId, regType));

		ServerEntity* entity = &mEntityMap[entityId];
		
		mEntityMapList[regType].emplace_back(entity);
		return entity;
	}

	return nullptr;
}

bool ServerEntityManagerHelper::RemoveEntity(uint32_t entityId)
{
	if (mEntityMap.contains(entityId))
	{
		ServerEntity* entity = &mEntityMap[entityId];

		unique_lock<shared_mutex> ulock(oMapMutex);

		mEntityMapList[entity->GetServerType()].remove(entity);

		DNPrint(0, LoggerLevel::Debug, "offline destory entity");
		mEntityMap.erase(entityId);
		return true;
	}

	return false;
}

void ServerEntityManagerHelper::MountEntity(ServerType type, ServerEntity* entity)
{
	unique_lock<shared_mutex> ulock(oMapMutex);
	if (mEntityMap.count(entity->ID()))
	{
		mEntityMapList[type].push_back(entity);
	}
}

void ServerEntityManagerHelper::UnMountEntity(ServerType type, ServerEntity* entity)
{
	unique_lock<shared_mutex> ulock(oMapMutex);
	mEntityMapList[type].remove(entity);
}

ServerEntity* ServerEntityManagerHelper::GetEntity(uint32_t entityId)
{
	shared_lock<shared_mutex> lock(oMapMutex);
	if (mEntityMap.contains(entityId))
	{
		return &mEntityMap[entityId];
	}
	// allow return empty
	return nullptr;
}

const list<ServerEntity*>& ServerEntityManagerHelper::GetEntityByList(ServerType type)
{
	shared_lock<shared_mutex> lock(oMapMutex);
	return mEntityMapList[type];
}

uint32_t ServerEntityManagerHelper::ServerIndex()
{
	return ++iServerId;
}
