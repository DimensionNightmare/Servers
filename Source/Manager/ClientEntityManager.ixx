module;
#include <map>
#include <list>
#include <shared_mutex>

#include "StdAfx.h"
export module ClientEntityManager;

import DNServer;
export import ClientEntity;
import EntityManager;

using namespace std;

export class ClientEntityManager : public EntityManager<ClientEntity>
{
public:
    ClientEntityManager(){};

	virtual ~ClientEntityManager(){};

	virtual bool Init() override;

public: // dll override
	

protected: // dll proxy
    

};

bool ClientEntityManager::Init()
{
	return EntityManager::Init();
}
