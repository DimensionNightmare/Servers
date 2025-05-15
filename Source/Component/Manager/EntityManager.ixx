module;
export module EntityManager;

export import ECSW;
import ThirdParty.Libhv;
import Logger;

export template<class TEntity = Entity>
class EntityManager : public Component
{
protected:
	/// @brief timer manager create
	EntityManager(System::WPtr system):Component(system)
	{
		pLoop = std::make_unique<hv::EventLoopThread>();

		pLogger = GetOwner()->GetWorld()->GetSystem<LoggerPrint>(EMSystemType::LoggerPrint);
	}
	
public:

	virtual ~EntityManager()
	{
		
	}

	/// @brief start timer manager
	virtual bool Start()
	{
		pLoop->start();
		return true;
	}

	/// @brief main loop func mount
	virtual void TickMainFrame() = 0;

	virtual void Dispose() override
	{
		Component::Dispose();

		pLoop = nullptr;
		for (auto& [id, entity] : mEntityMap)
		{
			entity->Dispose();
		}
		
		mEntityMap.clear();
	}

protected:

	LoggerPrint::Ptr GetLogger(){ return pLogger.lock(); }
public: // dll override

	const hv::EventLoopPtr& Timer() { return pLoop->loop(); }

	void AddTimerRecord(size_t timerId, uint32_t id)
	{
		std::unique_lock<std::shared_mutex> ulock(oTimerMutex);
		mMapTimer.emplace(timerId, id);
	}
	
protected: // dll proxy

	std::unordered_map<uint32_t, std::shared_ptr<TEntity>> mEntityMap;
	/// @brief mEntityMap Mutex
	std::shared_mutex oMapMutex;
	//
	std::unordered_map<uint64_t, uint32_t> mMapTimer;
	/// @brief mMapTimer Mutex
	std::shared_mutex oTimerMutex;

	std::unique_ptr<hv::EventLoopThread> pLoop;

	LoggerPrint::WPtr pLogger;

};
