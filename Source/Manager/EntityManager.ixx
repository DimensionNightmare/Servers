module;
export module EntityManager;

import Entity;
import ThirdParty.Libhv;
import std.compat;

export template<class TEntity = Entity>
class EntityManager
{
	
public:
	/// @brief timer manager create
	EntityManager()
	{
		pLoop = std::make_shared<EventLoopThread>();
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
		std::unique_lock<std::shared_mutex> ulock(oTimerMutex);
		mMapTimer.emplace(timerId, id);
	}
	
protected: // dll proxy

	std::unordered_map<uint32_t, TEntity> mEntityMap;
	/// @brief mEntityMap Mutex
	std::shared_mutex oMapMutex;
	//
	std::unordered_map<uint64_t, uint32_t> mMapTimer;
	/// @brief mMapTimer Mutex
	std::shared_mutex oTimerMutex;

	std::shared_ptr<EventLoopThread> pLoop;

};
