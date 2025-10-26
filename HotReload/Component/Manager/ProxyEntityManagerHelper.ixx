export module ProxyEntityManagerHelper;

import ProxyEntityManager;
import ProxyEntityHelper;
import FuncUtils;

export class ProxyEntityManagerHelper : public Helper<ProxyEntityManagerHelper, ProxyEntityManager>
{

private:

	ProxyEntityManagerHelper() = delete;
	~ProxyEntityManagerHelper() = default;

public:

	ProxyEntityHelper::Ptr AddEntity(size_t entityId)
	{
		if (!mEntityMap.contains(entityId))
		{
			ProxyEntity::Ptr entity = Base()->AddEntity(entityId);
			return entity->GetSelf<ProxyEntityHelper>();
		}

		return nullptr;
	}

	ProxyEntityHelper::Ptr GetEntity(size_t entityId)
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
