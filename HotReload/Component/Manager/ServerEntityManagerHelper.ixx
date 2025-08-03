export module ServerEntityManagerHelper;

import ServerEntityManager;
import DllUtils;
import ServerEntityHelper;

#define FUNCPLACE(class, func) &class::func, #class"_"#func

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
	using CVPtr = const Ptr&;

	ServerEntityHelper::Ptr GetEntity(uint64_t entityId)
	{
		std::shared_lock lock(oMapMutex);
		if (mEntityMap.contains(entityId))
		{
			return mEntityMap[entityId]->GetSelf<ServerEntityHelper>();
		}
		// allow return empty
		return nullptr;
	}

	ServerEntityHelper::Ptr AddEntity(uint64_t entityId, EMServerType regType)
	{
		if (!mEntityMap.contains(entityId))
		{
			TickMainSpaceDll(this, FUNCPLACE(ServerEntityManager,AddEntity), entityId, regType);
			
			ServerEntityHelper::Ptr entity = GetEntity(entityId);
			entity->SetServerType(regType);
			return entity;
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
