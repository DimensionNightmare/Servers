module;
#include "hv/Channel.h"

#include <map>
export module EntityManagerHelper;

import EntityManager;

using namespace std;
using namespace hv;

export template<class TEntity>
class EntityManagerHelper : public EntityManager<TEntity>
{
private:
	EntityManagerHelper(){}
public:
	template<class CastTEntity>
    CastTEntity* AddEntity(const SocketChannelPtr& channel, int entityId);

    void RemoveEntity(const SocketChannelPtr& channel);
};

template <class TEntity>
template <class CastTEntity>
CastTEntity* EntityManagerHelper<TEntity>::AddEntity(const SocketChannelPtr& channel, int entityId)
{
	TEntity* entity = nullptr;
	if (this->mEntityMap.contains(entityId))
	{
		entity = this->mEntityMap[entityId];
	}
	else
	{
		entity = make_shared<TEntity>();
		this->mEntityMap.emplace(entityId, entity);
		channel->setContext(entity);

		CastTEntity* castEntity = static_cast<CastTEntity*>(entity);
		castEntity->ID() = entityId;
		castEntity->Sock() = channel;
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