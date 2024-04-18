module;
#include <map>
#include "hv/Channel.h"
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
    TEntity* AddEntity(const SocketChannelPtr& channel, int entityId);

    void RemoveEntity(const SocketChannelPtr& channel);
};

template <class TEntity>
TEntity* EntityManagerHelper<TEntity>::AddEntity(const SocketChannelPtr& channel, int entityId)
{
	TEntity* entity = nullptr;
	if (this->mEntityMap.contains(entityId))
	{
		entity = &this->mEntityMap[entityId];
	}
	else
	{
		entity = &this->mEntityMap[entityId];
		channel->setContext(entity);

		entity->ID() = entityId;
		entity->Sock() = channel;
	}

	return entity;
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