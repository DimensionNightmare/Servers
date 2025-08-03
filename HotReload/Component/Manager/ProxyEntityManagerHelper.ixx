export module ProxyEntityManagerHelper;

import ProxyEntityManager;
import DllUtils;
import ProxyEntityHelper;

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
	using CVPtr = const Ptr&;

	ProxyEntityHelper::Ptr AddEntity(uint64_t entityId)
	{
		if (!mEntityMap.contains(entityId))
		{
			TickMainSpaceDll(this, FUNCPLACE(ProxyEntityManager,AddEntity), entityId);

			ProxyEntityHelper::Ptr entity = GetEntity(entityId);
			
			return entity;
		}

		return nullptr;
	}

	ProxyEntityHelper::Ptr GetEntity(uint64_t entityId)
	{
		std::shared_lock lock(oMapMutex);
		if (mEntityMap.contains(entityId))
		{
			return mEntityMap[entityId]->GetSelf<ProxyEntityHelper>();
		}
		// allow return empty
		return nullptr;
	}
};
