module;
export module ServerEntityManagerHelper;

import ServerEntityManager;
import ServerEntityHelper;
import Logger;

export class ServerEntityManagerHelper : public ServerEntityManager
{

private:

	ServerEntityManagerHelper() = delete;
	~ServerEntityManagerHelper() = default;

	ServerEntityManagerHelper(const ServerEntityManagerHelper&) = delete;
	void operator=(const ServerEntityManagerHelper&) = delete;

	ServerEntityManagerHelper(ServerEntityManagerHelper&&) = delete;
	ServerEntityManagerHelper& operator=(ServerEntityManagerHelper&&) = delete;

	void* operator new(size_t) = delete;
    void operator delete(void*) = delete;
public:
	using Ptr = std::shared_ptr<ServerEntityManagerHelper>;

	ServerEntity::Ptr AddEntity(uint64_t entityId, EMServerType regType)
	{
		if (!mEntityMap.contains(entityId))
		{
			std::unique_lock<std::shared_mutex> ulock(oMapMutex);

			// mEntityMap.emplace(std::piecewise_construct,
			// 	std::forward_as_tuple(entityId),
			// 	std::forward_as_tuple(entityId, regType));

			ServerEntity::Ptr entity = mEntityMap[entityId];

			mEntityMapList[regType].emplace_back(entity);
			return entity;
		}

		return nullptr;
	}

	bool RemoveEntity(uint64_t entityId)
	{
		if (mEntityMap.contains(entityId))
		{
			ServerEntity::Ptr entity = mEntityMap[entityId];

			std::unique_lock<std::shared_mutex> ulock(oMapMutex);

			mEntityMapList[entity->GetServerType()].remove(entity);

			SPidLogger.Record(ELogLevel_Debug, "offline destory entity");
			mEntityMap.erase(entityId);
			return true;
		}

		return false;
	}

	void MountEntity(EMServerType type, ServerEntity::Ptr entity)
	{
		std::unique_lock<std::shared_mutex> ulock(oMapMutex);
		if (mEntityMap.contains(entity->ID()))
		{
			mEntityMapList[type].emplace_back(entity);
		}
	}

	void UnMountEntity(EMServerType type, ServerEntity::Ptr entity)
	{
		std::unique_lock<std::shared_mutex> ulock(oMapMutex);
		mEntityMapList[type].remove(entity);
	}

	ServerEntity::Ptr GetEntity(uint64_t entityId)
	{
		std::shared_lock<std::shared_mutex> lock(oMapMutex);
		if (mEntityMap.contains(entityId))
		{
			return mEntityMap[entityId];
		}
		// allow return empty
		return nullptr;
	}

	const std::list<ServerEntity::Ptr>& GetEntitysByType(EMServerType type)
	{
		std::shared_lock<std::shared_mutex> lock(oMapMutex);
		return mEntityMapList[type];
	}

	[[nodiscard]] uint64_t GenServerId()
	{
		return ++iServerGenId;
	}
};
