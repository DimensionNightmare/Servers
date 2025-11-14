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
		pTimer = GetWorld()->GetSystemW<Timer>(EMSystemType::Timer);
	}
	
public:

	virtual ~EntityManager()
	{
		
	}

	virtual void Dispose() override
	{
		auto entitys = mEntityMap 
			| std::views::values;
			
		for (const auto& entity : entitys)
		{
			entity->Dispose();
		}
		
		mEntityMap.clear();

		Component::Dispose();
	}

	Timer::Ptr GetTimer(){ return pTimer.expired() ? nullptr : pTimer.lock(); }

public: // dll override

	void AddTimerRecord(size_t timerId, size_t id)
	{
		std::unique_lock ulock(oTimerMutex);
		mMapTimer.emplace(timerId, id);
	}
	
protected: // dll proxy

	std::unordered_map<size_t, std::shared_ptr<TEntity>> mEntityMap;
	/// @brief mEntityMap Mutex
	std::shared_mutex oMapMutex;
	//
	std::unordered_map<size_t, size_t> mMapTimer;
	/// @brief mMapTimer Mutex
	std::shared_mutex oTimerMutex;

	Timer::WPtr pTimer;

};
