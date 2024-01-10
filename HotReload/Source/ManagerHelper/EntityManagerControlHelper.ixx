module;
#include "hv/Channel.h"

#include <map>
export module EntityManagerControlHelper;

import ServerEntity;
import ServerEntityHelper;
import EntityManager;

using namespace std;
using namespace hv;

export template<class TEntity = ServerEntity>
class EntityManagerControlHelper : public EntityManager<TEntity>
{
private:
	EntityManagerControlHelper(){}
public:
	template<class CastTEntity = ServerEntityHelper>
    CastTEntity* AddEntity(const SocketChannelPtr& channel, int entityId);

    void RemoveEntity(const SocketChannelPtr& channel);

	template<class CastTEntity = ServerEntityHelper>
    CastTEntity* GetEntity(const string& ip);
};

template <class TEntity>
template <class CastTEntity>
CastTEntity* EntityManagerControlHelper<TEntity>::AddEntity(const SocketChannelPtr& channel, int entityId)
{
	TEntity* entity = nullptr;
	if (EntityManager<TEntity>::mEntityMap.count(entityId))
	{
		entity = EntityManager<TEntity>::mEntityMap[entityId];
	}
	else
	{
		entity = new TEntity;
		EntityManager<TEntity>::mEntityMap.emplace(entityId, entity);
		channel->setContext(entity);
	}

	return static_cast<CastTEntity*>(entity);
}

template <class TEntity>
void EntityManagerControlHelper<TEntity>::RemoveEntity(const SocketChannelPtr& channel)
{
	if(TEntity* entity = channel->getContext<TEntity>())
	{
		channel->setContext(nullptr);
		EntityManager<TEntity>::mEntityMap.erase(entity->iId);
		delete entity;
	}
	
}

template <class TEntity>
template <class CastTEntity>
CastTEntity *EntityManagerControlHelper<TEntity>::GetEntity(const string& ip)
{
	for(auto &[k,v]: EntityManager<TEntity>::mEntityMap)
	{
		auto castObj = static_cast<CastTEntity*>(v);
		if(castObj->GetServerIp() == ip)
			return castObj;
	}
	return nullptr;
}