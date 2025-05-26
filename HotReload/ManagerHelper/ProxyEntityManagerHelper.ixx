module;
export module ProxyEntityManagerHelper;

import ProxyEntityManager;
import DllUtils;

#define FUNCPLACE(class, func) &class::func, #class"_"#func

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
			TickMainSpaceDll(this, FUNCPLACE(ProxyEntityManager,AddEntity), entityId);

			ProxyEntity::Ptr entity = mEntityMap[entityId];
			
			return entity;
		}

		return nullptr;
	}

	bool RemoveEntity(uint64_t entityId)
	{
		if (mEntityMap.contains(entityId))
		{
			GetLogger()->Record(ELogLevel_Debug, "destory Proxy entity");
			ProxyEntity::Ptr& entity = mEntityMap[entityId];
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
