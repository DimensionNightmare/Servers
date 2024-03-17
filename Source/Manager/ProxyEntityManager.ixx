module;

#include <map>
#include <shared_mutex>
export module ProxyEntityManager;

import DNServer;
export import ProxyEntity;
import EntityManager;

using namespace std;

export template<class TEntity = ProxyEntity>
class ProxyEntityManager : public EntityManager<TEntity>
{
public:
    ProxyEntityManager();

	virtual ~ProxyEntityManager();

public: // dll override

protected: // dll proxy
	map<unsigned int, TEntity* > mEntityMapList;

	shared_mutex oMapMutex;
};



template <class TEntity>
ProxyEntityManager<TEntity>::ProxyEntityManager()
{
	
}

template <class TEntity>
ProxyEntityManager<TEntity>::~ProxyEntityManager()
{
	
}