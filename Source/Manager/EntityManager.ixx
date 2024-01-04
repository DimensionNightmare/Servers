module;
#include "hv/Channel.h"
#include <map>
export module EntityManager;

import Entity;

using namespace std;
using namespace hv;

export template<class TEntity = Entity>
class EntityManager
{
public:
    EntityManager();

	~EntityManager();

public: // dll override

protected: // dll proxy
    map<int, TEntity*> mEntityMap;
};

module:private;

template <class TEntity>
EntityManager<TEntity>::EntityManager()
{
	mEntityMap.clear();
}

template <class TEntity>
EntityManager<TEntity>::~EntityManager()
{
	for(auto& [k,v] : mEntityMap)
	{
		delete v;
		v = nullptr;
	}

	mEntityMap.clear();
}