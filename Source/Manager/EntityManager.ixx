module;

#include <map>
export module EntityManager;

import Entity;

using namespace std;

export template<class TEntity = Entity>
class EntityManager
{
public:
    EntityManager();

	virtual ~EntityManager();

public: // dll override

protected: // dll proxy
    map<unsigned int, TEntity*> mEntityMap;
};



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