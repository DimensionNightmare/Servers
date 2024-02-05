module;
#include "hv/Channel.h"

#include <map>
export module EntityManagerHelper;

import Entity;
import EntityHelper;
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
	if (this->mEntityMap.count(entityId))
	{
		entity = this->mEntityMap[entityId];
	}
	else
	{
		entity = make_shared<TEntity>();
		this->mEntityMap.emplace(entityId, entity);
		channel->setContext(entity);

		CastTEntity* castEntity = static_cast<CastTEntity*>(entity);
		castEntity->SetID(entityId);
		castEntity->SetSock(channel);
	}

	return static_cast<CastTEntity*>(entity);
}

template <class TEntity>
void EntityManagerHelper<TEntity>::RemoveEntity(const SocketChannelPtr& channel)
{
	if(TEntity* entity = channel->getContext<TEntity>())
	{
		channel->setContext(nullptr);
		this->mEntityMap.erase(entity->iId);
		delete entity;
	}
	
}