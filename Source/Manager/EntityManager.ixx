module;
#include "hv/EventLoopThread.h"

#include <map>
#include <shared_mutex>
export module EntityManager;

import Entity;

using namespace std;
using namespace hv;

export template<class TEntity = Entity>
class EntityManager
{
public:
    EntityManager();

	virtual ~EntityManager();

	virtual bool Init();

public: // dll override
	const EventLoopPtr& Timer(){return pLoop->loop();}
	void AddTimerRecord(size_t timerId, unsigned int id);

protected: // dll proxy
    map<unsigned int, TEntity*> mEntityMap;
	shared_mutex oMapMutex;
	//
	map<uint64_t, unsigned int > mMapTimer;
	shared_mutex oTimerMutex;

	EventLoopThread* pLoop;
};



template <class TEntity>
EntityManager<TEntity>::EntityManager()
{
	mEntityMap.clear();
	mMapTimer.clear();
	pLoop = new EventLoopThread;
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
	mMapTimer.clear();
	pLoop->stop(true);
}

template <class TEntity>
bool EntityManager<TEntity>::Init()
{
	pLoop->start(false);
	return true;
}

template <class TEntity>
void EntityManager<TEntity>::AddTimerRecord(size_t timerId, unsigned int id)
{
	mMapTimer.emplace(timerId, id);
}
