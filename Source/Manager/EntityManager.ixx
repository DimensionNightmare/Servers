module;
#include "StdMacro.h"
export module EntityManager;

import Entity;
import ThirdParty.Libhv;

export template<class TEntity = Entity>
class EntityManager
{
public:
	EntityManager();

	virtual ~EntityManager();

	virtual bool Init() = 0;

	virtual void TickMainFrame() = 0;

public: // dll override
	const EventLoopPtr& Timer() { return pLoop->loop(); }
	void AddTimerRecord(size_t timerId, uint32_t id);

protected: // dll proxy
	unordered_map<uint32_t, TEntity> mEntityMap;
	shared_mutex oMapMutex;
	//
	unordered_map<uint64_t, uint32_t > mMapTimer;
	shared_mutex oTimerMutex;

	shared_ptr<EventLoopThread> pLoop;
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
	unique_lock<shared_mutex> ulock(oTimerMutex);
	mMapTimer.emplace(timerId, id);
}
