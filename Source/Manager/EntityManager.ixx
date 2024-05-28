module;
#include <unordered_map>
#include <shared_mutex>
#include <cstdint>
#include "hv/EventLoopThread.h"
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
	const EventLoopPtr& Timer() { return pLoop->loop(); }
	void AddTimerRecord(size_t timerId, uint32_t id);

protected: // dll proxy
	unordered_map<uint32_t, TEntity> mEntityMap;
	shared_mutex oMapMutex;
	//
	unordered_map<uint64_t, uint32_t > mMapTimer;
	shared_mutex oTimerMutex;

	EventLoopThreadPtr pLoop;
};



template <class TEntity>
EntityManager<TEntity>::EntityManager()
{
	pLoop = make_shared<EventLoopThread>();
}

template <class TEntity>
EntityManager<TEntity>::~EntityManager()
{
	pLoop = nullptr;
	mEntityMap.clear();
	mMapTimer.clear();
}

template <class TEntity>
bool EntityManager<TEntity>::Init()
{
	pLoop->start();
	return true;
}

template <class TEntity>
void EntityManager<TEntity>::AddTimerRecord(size_t timerId, uint32_t id)
{
	mMapTimer.emplace(timerId, id);
}
