module;
#include "hv/Channel.h"
#include <map>
export module ActorManager;

export import Entity;

using namespace std;
using namespace hv;

export class ActorManager;

class ActorManager
{
public:
    ActorManager();
    void AddEntity(SocketChannel* channel, int entityId);
    void RemoveEntity(SocketChannel *channel);

public:
    map<int, Entity*> mEntityMap;
};

module:private;

ActorManager::ActorManager()
{
    mEntityMap.clear();
}

void ActorManager::AddEntity(SocketChannel* channel, int entityId)
{
    Entity* entity = new Entity;
    entity->iId = entityId;
    mEntityMap.emplace(entityId, entity);
    channel->setContext(entity);
}

void ActorManager::RemoveEntity(SocketChannel* channel)
{
    if(Entity* entity = channel->getContext<Entity>())
    {
        channel->setContext(nullptr);
        mEntityMap.erase(entity->iId);
        delete entity;
    }
    
}