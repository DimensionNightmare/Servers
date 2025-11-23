export module EntityManager;

import Logger;
import ECSW;
import Timer;

export template<class TEntity = Entity>
class EntityManager : public Component
{
protected:
	/// @brief timer manager create
	EntityManager(System::CVPtr system):Component(system)
	{
		pTimer = GetWorld()->GetSystem<Timer>(EMSystemType::Timer);
	}
	
public:

	virtual ~EntityManager()
	{
		
	}

	virtual void Dispose() override
	{
		auto entitys = mEntityMap 
			| std::views::keys
			| std::ranges::to<std::vector<size_t>>();
			
		for (const auto& entityId : entitys)
		{
			DisposeEntity(mEntityMap[entityId]);
		}
		
		mEntityMap.clear();

		Component::Dispose();
	}

	Timer::Ptr GetTimer(){ return pTimer.expired() ? nullptr : pTimer.lock(); }

	template<typename T>
	void DisposeEntity(const std::shared_ptr<T>& entity)
	{
		int useCount = entity.use_count();
		if (auto temp = RemoveEntity(entity->ID()))
		{
			if(useCount == 1)
			{
				temp->Dispose();
				return;
			}
		}

		entity->Dispose();
	}

protected: 

	void AddTimerRecord(size_t timerId, size_t id)
	{
		std::unique_lock ulock(oTimerMutex);
		mMapTimer.emplace(timerId, id);
	}

	virtual TEntity::Ptr RemoveEntity(size_t entityId) = 0;
	
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
