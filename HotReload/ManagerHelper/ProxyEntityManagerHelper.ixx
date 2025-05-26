module;
export module ProxyEntityManagerHelper;

import ProxyEntityHelper;
import ProxyEntityManager;
import Logger;

export class ProxyEntityManagerHelper : public ProxyEntityManager
{

private:

	ProxyEntityManagerHelper() = delete;
	~ProxyEntityManagerHelper() = default;

	ProxyEntityManagerHelper(const ProxyEntityManagerHelper&) = delete;
	void operator=(const ProxyEntityManagerHelper&) = delete;

	ProxyEntityManagerHelper(ProxyEntityManagerHelper&&) = delete;
	ProxyEntityManagerHelper& operator=(ProxyEntityManagerHelper&&) = delete;

	void* operator new(size_t) = delete;
    void operator delete(void*) = delete;
public:
	using Ptr = std::shared_ptr<ProxyEntityManagerHelper>;

	ProxyEntity::Ptr AddEntity(uint64_t entityId)
	{
		if (!mEntityMap.contains(entityId))
		{
			ProxyEntity::Ptr entity = std::shared_ptr<ProxyEntity>(new ProxyEntity(GetOwner()->GetWorldW()));
			entity->SetID(entityId);

			std::unique_lock<std::shared_mutex> ulock(oMapMutex);
			mEntityMap[entityId] = entity;
			return entity;
		}

		return nullptr;
	}

	bool RemoveEntity(uint64_t entityId)
	{
		if (mEntityMap.contains(entityId))
		{
			SPidLogger.Record(ELogLevel_Debug, "destory Proxy entity");
			ProxyEntity::Ptr entity = mEntityMap[entityId];
			entity->Dispose();

			std::unique_lock<std::shared_mutex> ulock(oMapMutex);
			mEntityMap.erase(entityId);
			return true;
		}

		return false;
	}

	ProxyEntity::Ptr GetEntity(uint64_t entityId)
	{
		std::shared_lock<std::shared_mutex> lock(oMapMutex);
		if (mEntityMap.contains(entityId))
		{
			return mEntityMap[entityId];
		}
		// allow return empty
		return nullptr;
	}
};
