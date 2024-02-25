module;

#include <map>
#include <list>
#include <shared_mutex>
export module ServerEntityManager;

import DNServer;
export import ServerEntity;
import EntityManager;

using namespace std;

export template<class TEntity = ServerEntity>
class ServerEntityManager : public EntityManager<ServerEntity>
{
public:
    ServerEntityManager();

	virtual ~ServerEntityManager();

public: // dll override

protected: // dll proxy
    map<ServerType, list<TEntity*> > mEntityMapList;
	// server pull server
	atomic<unsigned int> iServerId;
	list<unsigned int> mIdleServerId;

	shared_mutex oMapMutex;
};

module:private;

template <class TEntity>
ServerEntityManager<TEntity>::ServerEntityManager()
{
	mEntityMapList.clear();
	iServerId = 0;
	mIdleServerId.clear();
}

template <class TEntity>
ServerEntityManager<TEntity>::~ServerEntityManager()
{
	mEntityMapList.clear();
}