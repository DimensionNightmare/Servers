module;
#include "StdMacro.h"
export module EntityManager;

import Entity;
import ThirdParty.Libhv;

export template<class TEntity = Entity>
class EntityManager
{
public:
	/// @brief timer manager create
	EntityManager()
	{
		pLoop = make_shared<EventLoopThread>();
	}

	virtual ~EntityManager()
	{
		pLoop = nullptr;
		mEntityMap.clear();
	}

	/// @brief start timer manager
	virtual bool Init()
	{
		pLoop->start();
		return true;
	}

	/// @brief main loop func mount
	virtual void TickMainFrame() = 0;

public: // dll override

	const EventLoopPtr& Timer() { return pLoop->loop(); }

	void AddTimerRecord(size_t timerId, uint32_t id)
	{
		unique_lock<shared_mutex> ulock(oTimerMutex);
		mMapTimer.emplace(timerId, id);
	}
	
protected: // dll proxy

	unordered_map<uint32_t, TEntity> mEntityMap;
	/// @brief mEntityMap Mutex
	shared_mutex oMapMutex;
	//
	unordered_map<uint64_t, uint32_t > mMapTimer;
	/// @brief mMapTimer Mutex
	shared_mutex oTimerMutex;

	shared_ptr<EventLoopThread> pLoop;

};
