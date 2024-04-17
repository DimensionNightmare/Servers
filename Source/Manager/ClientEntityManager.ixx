module;
#include "StdAfx.h"

#include <map>
#include <list>
#include <shared_mutex>
export module ClientEntityManager;

import DNServer;
export import ClientEntity;
import EntityManager;

using namespace std;

export template<class TEntity = ClientEntity>
class ClientEntityManager : public EntityManager<ClientEntity>
{
public:
    ClientEntityManager();

	virtual ~ClientEntityManager();

	virtual bool Init() override;

public: // dll override
	

protected: // dll proxy
    

};



template <class TEntity>
ClientEntityManager<TEntity>::ClientEntityManager()
{
	
}

template <class TEntity>
ClientEntityManager<TEntity>::~ClientEntityManager()
{
	
}

template <class TEntity>
bool ClientEntityManager<TEntity>::Init()
{
	return EntityManager<TEntity>::Init();
}
