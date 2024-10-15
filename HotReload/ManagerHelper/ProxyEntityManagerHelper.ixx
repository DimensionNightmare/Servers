module;
#include "StdMacro.h"
export module ProxyEntityManagerHelper;

export import ProxyEntityHelper;
export import ProxyEntityManager;
import Logger;

export class ProxyEntityManagerHelper : public ProxyEntityManager
{
private:
	ProxyEntityManagerHelper() = delete;
public:
	ProxyEntity* AddEntity(uint32_t entityId);

	bool RemoveEntity(uint32_t entityId);

	ProxyEntity* GetEntity(uint32_t id);
};

ProxyEntity* ProxyEntityManagerHelper::AddEntity(uint32_t entityId)
{
	if (!mEntityMap.contains(entityId))
	{
		unique_lock<shared_mutex> ulock(oMapMutex);
		mEntityMap.emplace(std::piecewise_construct,
			std::forward_as_tuple(entityId),
			std::forward_as_tuple(entityId));

		return &mEntityMap[entityId];
	}

	return nullptr;
}

bool ProxyEntityManagerHelper::RemoveEntity(uint32_t entityId)
{
	if (mEntityMap.contains(entityId))
	{
		unique_lock<shared_mutex> ulock(oMapMutex);

		DNPrint(0, LoggerLevel::Debug, "destory Proxy entity");
		mEntityMap.erase(entityId);
		return true;
	}

	return false;
}

ProxyEntity* ProxyEntityManagerHelper::GetEntity(uint32_t entityId)
{
	shared_lock<shared_mutex> lock(oMapMutex);
	if (mEntityMap.contains(entityId))
	{
		return &mEntityMap[entityId];
	}
	// allow return empty
	return nullptr;
}
