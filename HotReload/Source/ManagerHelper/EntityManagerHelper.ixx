module;
#include "hv/Channel.h"
#include <map>
export module EntityManagerHelper;

import Entity;
import EntityHelper;
import ServerEntityHelper;
import EntityManager;

using namespace std;
using namespace hv;

export template<class TEntity = Entity>
class EntityManagerHelper : public EntityManager<TEntity>
{
private:
	EntityManagerHelper(){}
public:
	template<class CastTEntity = EntityHelper>
    CastTEntity* AddEntity(const SocketChannelPtr& channel, int entityId);

    void RemoveEntity(const SocketChannelPtr& channel);
};

template <class TEntity>
template <class CastTEntity>
CastTEntity* EntityManagerHelper<TEntity>::AddEntity(const SocketChannelPtr& channel, int entityId)
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
void EntityManagerHelper<TEntity>::RemoveEntity(const SocketChannelPtr& channel)
{
	if(TEntity* entity = channel->getContext<TEntity>())
	{
		channel->setContext(nullptr);
		EntityManager<TEntity>::mEntityMap.erase(entity->iId);
		delete entity;
	}
	
}