module;

#include <map>
#include <list>
export module ServerEntityManager;

import DNServer;
import ServerEntity;
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
};

module:private;

template <class TEntity>
ServerEntityManager<TEntity>::ServerEntityManager()
{
	mEntityMapList.clear();
}

template <class TEntity>
ServerEntityManager<TEntity>::~ServerEntityManager()
{
	mEntityMapList.clear();
}