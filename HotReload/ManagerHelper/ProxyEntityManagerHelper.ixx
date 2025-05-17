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

	ProxyEntity::Ptr AddEntity(uint32_t entityId)
	{
		if (!mEntityMap.contains(entityId))
		{
			std::unique_lock<std::shared_mutex> ulock(oMapMutex);
			// mEntityMap.emplace(std::piecewise_construct,
			// 	std::forward_as_tuple(entityId),
			// 	std::forward_as_tuple(entityId));

			return mEntityMap[entityId];
		}

		return nullptr;
	}

	bool RemoveEntity(uint64_t entityId)
	{
		if (mEntityMap.contains(entityId))
		{
			std::unique_lock<std::shared_mutex> ulock(oMapMutex);

			SPidLogger.Record(ELogLevel_Debug, "destory Proxy entity");
			mEntityMap.erase(entityId);
			return true;
		}

		return false;
	}

	ProxyEntity::Ptr GetEntity(uint32_t entityId)
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
