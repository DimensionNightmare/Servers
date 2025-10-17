export module EntityManager;

import Logger;
import ECSW;
import ThirdParty.Libhv;
import Timer;

export template<class TEntity = Entity>
class EntityManager : public Component
{
protected:
	/// @brief timer manager create
	EntityManager(System::WPtr system):Component(system)
	{
		pTimer = GetOwner()->GetWorld()->GetSystemW<Timer>(EMSystemType::Timer);
	}
	
public:

	virtual ~EntityManager()
	{
		
	}

	/// @brief main loop func mount
	virtual void TickMainFrame() = 0;

	virtual void Dispose() override
	{
		for (auto& [id, entity] : mEntityMap)
		{
			entity->Dispose();
		}
		
		mEntityMap.clear();

		Component::Dispose();
	}

	Timer::Ptr GetTimer(){ return pTimer.expired() ? nullptr : pTimer.lock(); }

public: // dll override

	void AddTimerRecord(uint64_t timerId, uint64_t id)
	{
		std::unique_lock ulock(oTimerMutex);
		mMapTimer.emplace(timerId, id);
	}
	
protected: // dll proxy

	std::unordered_map<uint64_t, std::shared_ptr<TEntity>> mEntityMap;
	/// @brief mEntityMap Mutex
	std::shared_mutex oMapMutex;
	//
	std::unordered_map<uint64_t, uint64_t> mMapTimer;
	/// @brief mMapTimer Mutex
	std::shared_mutex oTimerMutex;

	Timer::WPtr pTimer;

};
