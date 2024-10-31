module;
#include "StdMacro.h"
export module ServerEntityManagerHelper;

import ServerEntityManager;
import ServerEntityHelper;
import Logger;

export class ServerEntityManagerHelper : public ServerEntityManager
{

private:

	ServerEntityManagerHelper() {}
public:

	ServerEntity* AddEntity(uint32_t entityId, EMServerType regType)
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

	bool RemoveEntity(uint32_t entityId)
	{
		if (mEntityMap.contains(entityId))
		{
			ServerEntity* entity = &mEntityMap[entityId];

			unique_lock<shared_mutex> ulock(oMapMutex);

			mEntityMapList[entity->GetServerType()].remove(entity);

			DNPrint(0, EMLoggerLevel::Debug, "offline destory entity");
			mEntityMap.erase(entityId);
			return true;
		}

		return false;
	}

	void MountEntity(EMServerType type, ServerEntity* entity)
	{
		unique_lock<shared_mutex> ulock(oMapMutex);
		if (mEntityMap.contains(entity->ID()))
		{
			mEntityMapList[type].emplace_back(entity);
		}
	}

	void UnMountEntity(EMServerType type, ServerEntity* entity)
	{
		unique_lock<shared_mutex> ulock(oMapMutex);
		mEntityMapList[type].remove(entity);
	}

	ServerEntity* GetEntity(uint32_t entityId)
	{
		shared_lock<shared_mutex> lock(oMapMutex);
		if (mEntityMap.contains(entityId))
		{
			return &mEntityMap[entityId];
		}
		// allow return empty
		return nullptr;
	}

	const list<ServerEntity*>& GetEntitysByType(EMServerType type)
	{
		shared_lock<shared_mutex> lock(oMapMutex);
		return mEntityMapList[type];
	}

	[[nodiscard]] uint32_t GenServerId()
	{
		return ++iServerGenId;
	}
};
