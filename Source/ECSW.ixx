module;
export module ECSW;

export import std.compat;

#pragma region EnumType

export enum EMEventType : uint8_t
{
	None = 0			,
	ServerStart			,
	ServerStop			,
	ServerPause			,
	ServerResume		,
};

export enum class EMComponentType : uint8_t
{
	None,
	DNServerProxy,
	ServerEntityManager,
	DNClientProxy,
	DNWebProxy,
	RoomEntityManager,
	ProxyEntityManager,
	ClientEntityManager,
};

export enum class EMEntityType : uint8_t
{
	None,
	// NetEntity, virtual
	Server,
	Proxy,
	Client,
	Room,
};

export enum class EMSystemType : uint8_t
{
	None,
	LoggerPrint,
	DNl10n,
	DNServer,
	HotReloadDll,
};


#pragma endregion

class World;

#pragma region Event

class Event
{
public:
	Event() = default;
	~Event() = default;

	template<typename T, typename Callback>
	uint32_t AddEvent(EMEventType type, std::shared_ptr<T> entity, Callback&& callback)
	{
		uint32_t eventId = iEventGenId++;
		auto lumbdaFunc = [entity, callback = std::forward<Callback>(callback)](){ 
			if(entity && !entity->IsDispose())
			{
				(entity.get()->*callback)();
			}
		};
		mEventIdMap[eventId] = std::make_pair(type, lumbdaFunc);

		mEventCollection[type].push_back(eventId);
		return eventId;
	}

	void Broadcast(EMEventType type)
	{
		for(uint32_t eventId : mEventCollection[type]) 
		{
			auto& [type, func] = mEventIdMap[eventId];
			func();
		}
	}

private:
	std::atomic<uint32_t> iEventGenId;
	std::unordered_map<uint32_t, std::pair<EMEventType, std::function<void()> > > mEventIdMap;
	std::unordered_map<EMEventType, std::vector<uint32_t>> mEventCollection;
};

#pragma endregion


#pragma region Object

class Object : public std::enable_shared_from_this<Object>, public Event
{
public:
	using Ptr = std::shared_ptr<Object>;

	virtual ~Object()
	{
		bIsDisposed = true;
	}
	
public: // dll override

	uint32_t ID() { return iId; }

	virtual void Dispose() = 0;

	virtual bool Awake(){ return true;}

	template<typename T>
	std::shared_ptr<T> GetSelf() { return std::static_pointer_cast<T>(shared_from_this()); }

	bool IsDispose() { return bIsDisposed; }
	
private:

	uint32_t iId = 0;

	bool bIsDisposed = false;
};

#pragma endregion


#pragma region Component


export class Component : public Object
{
protected:
	friend class Entity;
	Component(std::shared_ptr<Entity> owner):
		pOwner(owner)
	{

	}
public:
	using Ptr = std::shared_ptr<Component>;

	/// @brief this component owner
	virtual ~Component()
	{
	}

	void Dispose()
	{
		pOwner = nullptr;
	}

	std::shared_ptr<Entity> GetOwner(){ return pOwner; }
public:

	EMComponentType GetComponentType() { return eComponentType; }

protected: // dll proxy

	EMComponentType eComponentType = EMComponentType::None;

	std::shared_ptr<Entity> pOwner;
};

#pragma endregion

#pragma region Entity

export class Entity : public Object
{
protected:
	Entity(std::shared_ptr<World> world):
		pWorld(world)
	{

	}

public:
	using Ptr = std::shared_ptr<Entity>;

	virtual ~Entity(){}
	
public: // dll override

	uint32_t ID() { return iId; }

	/// @brief entity type total enum
	EMEntityType GetEntityType() { return eEntityType; }

	std::shared_ptr<World> GetWorld(){ return pWorld; }

	template<typename T>
	std::shared_ptr<T> GetComponent(EMComponentType type)
	{
		for (auto& component : mComponents)
		{
			if (component->GetComponentType() == type)
			{
				return std::dynamic_pointer_cast<T>(component);
			}
		}
		return nullptr;
	}

	virtual void Dispose()
	{
		for (auto& component : mComponents)
		{
			component->Dispose();
		}

		mComponents.clear();

		pWorld = nullptr;
	}

	template<typename T>
	std::shared_ptr<T> AddComponent()
	{
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from component");
		try
		{
			std::shared_ptr<T> component = std::shared_ptr<T>(new T(shared_from_this()));
			if(!component->Awake())
			{
				component->Dispose();
				return nullptr;
			}
			mComponents.emplace_back(component);
			return component;
		}
		catch(const std::exception& e)
		{
			std::cerr << e.what() << '\n';
		}
		
		return nullptr;
	}

	template<typename T>
	std::shared_ptr<T> GetSelf()
	{
		std::static_pointer_cast<T>(shared_from_this());
	}
	
protected: // dll proxy

	EMEntityType eEntityType = EMEntityType::None;

private:

	std::shared_ptr<World> pWorld;


	uint32_t iId = 0;

	std::vector<std::shared_ptr<Component>> mComponents;
};


#pragma endregion

#pragma region System

export class System : public Entity
{
    friend class World;
public:
	using Ptr = std::shared_ptr<System>;

	virtual ~System()
	{
	}

	void Dispose()
	{
		
	}

	EMSystemType GetSystemType() { return emSystemType; }

	template<typename T>
	std::shared_ptr<T> AddComponent()
	{
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from component");
		try
		{
			std::shared_ptr<T> component = std::shared_ptr<T>(new T(GetSelf<System>()));
			if(!component->Awake())
			{
				component->Dispose();
				return nullptr;
			}
			mComponents.emplace_back(component);
			return component;
		}
		catch(const std::exception& e)
		{
			std::cerr << e.what() << '\n';
		}
		
		return nullptr;
	}

protected:
	
	System(std::shared_ptr<World> world) 
		: Entity(world)
	{
	}

	EMSystemType emSystemType = EMSystemType::None;
};


#pragma endregion

#pragma region World

export class World : public std::enable_shared_from_this<World>
{
public:
	using Ptr = std::shared_ptr<World>;

	virtual ~World() = default;

	template<typename T = System>
	std::shared_ptr<T> AddSystem()
	{
		static_assert(std::is_base_of_v<System, T>, "T must inherit from System");
		try
		{
			std::shared_ptr<T> system = std::shared_ptr<T>(new T(shared_from_this()));
			if(!system->Awake())
			{
				system->Dispose();
				return nullptr;
			}
			mSystemMap.emplace_back(system);
			return system;
		}
		catch(const std::exception& e)
		{
			std::cerr << e.what() << '\n';
		}
		
		return nullptr;
	}

	template<typename T = System>
	std::shared_ptr<T> GetSystem(EMSystemType type)
	{
		static_assert(std::is_base_of_v<System, T>, "T must inherit from System");
		try
		{
			for(auto& one : mSystemMap)
			{
				if(one->GetSystemType() == type)
				{
					return std::static_pointer_cast<T>(one);
				}
			}
		}
		catch(const std::exception& e)
		{
			std::cerr << e.what() << '\n';
		}
		
		return nullptr;
	}

	void Dispose()
	{
		for (auto& system : mSystemMap)
		{
			system->Dispose();
		}
		mSystemMap.clear();
	}

	void MoveLuanchConfigToSelf(std::unordered_map<std::string, std::string>&& config)
	{
		mLuanchConfig = std::move(config);
	}

	std::string* LaunchParam(const std::string& key)
	{
		if(mLuanchConfig.count(key))
		{
			return &mLuanchConfig[key];
		}

		return nullptr;
	}

private:
	std::unordered_map<std::string, std::string> mLuanchConfig;

	std::vector<std::shared_ptr<System>> mSystemMap;
};

#pragma endregion
