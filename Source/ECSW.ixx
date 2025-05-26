module;

export module ECSW;

import std.compat;
import ThirdParty.Platform;
import NumUtils;

#pragma region EnumType

export enum EMEventType : uint8_t
{
	None = 0			,
	ServerStart			,
	ServerStop			,
	ServerPause			,
	ServerResume		,
	AppStart			,
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
	MdbProxy,
	RdbProxy
};

export enum class EMEntityType : uint8_t
{
	None,
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

export class Event
{
public:
	template<typename T, typename Callback>
	void AddEvent(EMEventType type, std::weak_ptr<T> entity, Callback&& callback)
	{
		uint64_t objId = entity.lock()->ID();

		auto lumbdaFunc = [entity, callback](){ 
			if(auto origin = entity.lock())
			{
				(origin.get()->*callback)();
			}
		};

		mEventIdMap[objId][type] = lumbdaFunc;

		mEventCollection[type][objId] = objId;
	}

	void Broadcast(EMEventType type)
	{
		for(auto& [objId,_] : mEventCollection[type]) 
		{
			mEventIdMap[objId][type]();
		}
	}

	void RemoveEvent(uint64_t objId)
	{
		auto it = mEventIdMap.find(objId);
		if(it != mEventIdMap.end())
		{
			for(auto& [type, _] : it->second)
			{
				mEventCollection[type].erase(objId);
			}

			mEventIdMap.erase(it);
		}
	}

private:
	std::unordered_map<uint64_t, std::unordered_map<EMEventType, std::function<void()> > > mEventIdMap;
	std::unordered_map<EMEventType, std::unordered_map<uint64_t,uint64_t>> mEventCollection;
};

export Event GEvent;

#pragma endregion


#pragma region Object

export class Object : public std::enable_shared_from_this<Object>, public Event
{
public:
	using Ptr = std::shared_ptr<Object>;

	virtual ~Object()
	{
		if(!bIsDisposed)
		{
			// throw std::runtime_error("Object not disposed! Please check code!");
			std::cerr << "Object not disposed! Please check code!\n" << Platform::GetStackTrace() << std::endl;
		}
	}
	
public: // dll override

	uint64_t ID() { return iId; }

	void SetID(uint64_t id) { iId = id; }

	virtual void Dispose()
	{
		bIsDisposed = true;
	}

	virtual bool Awake(){ return true;}

	template<typename T>
	std::shared_ptr<T> GetSelf() { return std::static_pointer_cast<T>(shared_from_this()); }

	template<typename T>
	std::weak_ptr<T> GetSelfW() { return std::weak_ptr<T>(GetSelf<T>()); }

	bool IsDispose() { return bIsDisposed; }
	
protected:

	uint64_t iId = SFIdGenerator.nextId();

	bool bIsDisposed = false;
};

#pragma endregion


#pragma region Component


export class Component : public Object
{
protected:
	friend class Entity;
	Component(std::weak_ptr<Entity> owner):
		pOwner(owner)
	{

	}
public:
	using Ptr = std::shared_ptr<Component>;

	/// @brief this component owner
	virtual ~Component()
	{
	}

	virtual void Dispose()
	{
		Object::Dispose();
	}

	std::shared_ptr<Entity> GetOwner(){ return pOwner.expired() ? nullptr : pOwner.lock(); }

	template<typename T>
	std::shared_ptr<T> GetOwner(){ return std::static_pointer_cast<T>(GetOwner()); }
public:

	EMComponentType GetComponentType() { return eComponentType; }

protected: // dll proxy

	EMComponentType eComponentType = EMComponentType::None;

	std::weak_ptr<Entity> pOwner;
};

#pragma endregion

#pragma region Entity

export class Entity : public Object
{
protected:
	Entity(std::weak_ptr<World> world):
		pWorld(world)
	{

	}

public:
	using Ptr = std::shared_ptr<Entity>;

	virtual ~Entity()
	{
		
	}
	
public: // dll override

	/// @brief entity type total enum
	EMEntityType GetEntityType() { return eEntityType; }

	std::shared_ptr<World> GetWorld(){ return pWorld.expired() ? nullptr : pWorld.lock(); }

	std::weak_ptr<World> GetWorldW(){ return pWorld; }

	template<typename T>
	std::shared_ptr<T> GetComponent(EMComponentType type)
	{
		auto it = mComponents.find(type);
		if (it != mComponents.end())
		{
			return std::static_pointer_cast<T>(it->second);
		}
		
		return nullptr;
	}

	virtual void Dispose() override
	{
		Object::Dispose();

		for (auto& [type, component] : mComponents)
		{
			component->Dispose();
		}

		mComponents.clear();
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
			mComponents.emplace(component->GetComponentType(), component);
			return component;
		}
		catch(const std::exception& e)
		{
			std::cerr << e.what() << '\n';
		}
		
		return nullptr;
	}

	void RemoveComponent(EMComponentType type)
	{
		auto it = mComponents.find(type);
		if (it == mComponents.end())
		{
			return;
		}

		Event::RemoveEvent(it->second->ID());
		mComponents.erase(type);
	}
	
protected: // dll proxy

	EMEntityType eEntityType = EMEntityType::None;

	std::weak_ptr<World> pWorld;

	std::unordered_map<EMComponentType, std::shared_ptr<Component>> mComponents;
};


#pragma endregion

#pragma region System

export class System : public Entity
{
protected:
	friend class World;
	
	System(std::weak_ptr<World> world) 
		: Entity(world)
	{
	}

    
public:
	using Ptr = std::shared_ptr<System>;
	using WPtr = std::weak_ptr<System>;

	virtual ~System()
	{
	}

	virtual void Dispose() override
	{
		Entity::Dispose();
	}

	EMSystemType GetSystemType() { return emSystemType; }

	template<typename T>
	std::shared_ptr<T> AddComponent()
	{
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from component");
		try
		{
			std::shared_ptr<T> component = std::shared_ptr<T>(new T(GetSelfW<System>()));
			if(!component->Awake())
			{
				component->Dispose();
				return nullptr;
			}
			mComponents.emplace(component->GetComponentType(), component);
			return component;
		}
		catch(const std::exception& e)
		{
			std::cerr << e.what() << '\n';
		}
		
		return nullptr;
	}

protected:

	EMSystemType emSystemType = EMSystemType::None;
};


#pragma endregion

#pragma region World

export class World : public Object
{
public:
	using Ptr = std::shared_ptr<World>;
	using WPtr = std::weak_ptr<World>;

	virtual ~World()
	{

	}
	
	void AddSystem(System::Ptr system)
	{
		mSystemMap.emplace(system->GetSystemType(), system);
	}

	template<typename T = System>
	std::shared_ptr<T> AddSystem()
	{
		static_assert(std::is_base_of_v<System, T>, "T must inherit from System");
		try
		{
			std::shared_ptr<T> system = std::shared_ptr<T>(new T(GetSelfW<World>()));
			if(!system->Awake())
			{
				system->Dispose();
				return nullptr;
			}

			mSystemMap.emplace(system->GetSystemType(), system);
			return system;
		}
		catch(const std::exception& e)
		{
			std::cerr << e.what() << '\n';
		}
		
		return nullptr;
	}

	template<typename T = System>
	std::weak_ptr<T> GetSystemW(EMSystemType type)
	{
		static_assert(std::is_base_of_v<System, T>, "T must inherit from System");
		try
		{
			auto it = mSystemMap.find(type);
			if (it != mSystemMap.end())
			{
				auto ptr = std::static_pointer_cast<T>(it->second);
				return ptr ? ptr : std::weak_ptr<T>{};
			}
		}
		catch(const std::exception& e)
		{
			std::cerr << e.what() << '\n';
		}
		
		return {};
	}

	template<typename T = System>
	std::shared_ptr<T> GetSystem(EMSystemType type)
	{
		static_assert(std::is_base_of_v<System, T>, "T must inherit from System");
		try
		{
			auto it = mSystemMap.find(type);
			if (it != mSystemMap.end())
			{
				return std::static_pointer_cast<T>(it->second);
			}
		}
		catch(const std::exception& e)
		{
			std::cerr << e.what() << '\n';
		}
		
		return nullptr;
	}

	std::shared_ptr<System> RemoveSystem(EMSystemType type)
	{
		auto it = mSystemMap.find(type);
		if (it == mSystemMap.end())
		{
			return nullptr;
		}

		auto system = it->second;
		mSystemMap.erase(it);
		return system;
	}

	void Dispose()
	{
		Object::Dispose();
		
		for (auto& [_,system] : mSystemMap)
		{
			system->Dispose();
		}
		mSystemMap.clear();
	}

	void MoveLuanchConfigToSelf(std::unordered_map<std::string, std::string>&& config)
	{
		mLuanchConfig = std::move(config);
	}

	void MoveLuanchConfigToSelf(std::unordered_map<std::string, std::string>& config)
	{
		mLuanchConfig = config;
	}

	std::string* LaunchParam(const std::string& key)
	{
		if(mLuanchConfig.count(key))
		{
			return &mLuanchConfig[key];
		}

		return nullptr;
	}

	void RemoveBySystemSelf(EMSystemType type)
	{
		auto it = mSystemMap.find(type);
		if (it == mSystemMap.end())
		{
			return;
		}

		mSystemMap.erase(it);
	}

private:
	std::unordered_map<std::string, std::string> mLuanchConfig;

	std::unordered_map<EMSystemType, std::shared_ptr<System>> mSystemMap;
};

#pragma endregion
