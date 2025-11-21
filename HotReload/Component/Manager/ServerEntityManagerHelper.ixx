export module ServerEntityManagerHelper;

import ServerEntityManager;
import ServerEntityHelper;
import FuncUtils;

export class ServerEntityManagerHelper : public Helper<ServerEntityManagerHelper, ServerEntityManager>
{

private:

	ServerEntityManagerHelper() = delete;
	~ServerEntityManagerHelper() = default;

public:

	ServerEntityHelper::CVPtr GetEntity(size_t entityId)
	{
		std::shared_lock lock(oMapMutex);
		if (mEntityMap.contains(entityId))
		{
			return mEntityMap[entityId]->GetSelf<ServerEntityHelper>();
		}
		// allow return empty
		return nullptr;
	}

	ServerEntityHelper::Ptr AddEntity(size_t entityId, EMServerType regType)
	{
		if (!mEntityMap.contains(entityId))
		{
			ServerEntity::Ptr entity = GetBase()->AddEntity(entityId, regType);
			mEntityMapList[regType].emplace_back(entity);
			
			ServerEntityHelper::CVPtr helper = entity->GetSelf<ServerEntityHelper>();
			helper->SetServerType(regType);
			return helper;
		}

		return nullptr;
	}

	void MountEntity(ServerEntityHelper::CVPtr entity)
	{
		std::unique_lock ulock(oMapMutex);
		if (mEntityMap.contains(entity->ID()))
		{
			mEntityMapList[entity->GetServerType()].emplace_back(entity);
		}
	}

	void UnMountEntity(ServerEntityHelper::CVPtr entity)
	{
		std::unique_lock ulock(oMapMutex);
		mEntityMapList[entity->GetServerType()].remove(entity);
	}

	std::list<ServerEntity::Ptr>& GetEntitysByType(EMServerType type)
	{
		std::shared_lock lock(oMapMutex);
		return mEntityMapList[type];
	}

};
