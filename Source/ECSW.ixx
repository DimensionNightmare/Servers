export module ECSW;

import std.compat;
import ThirdParty.Platform;
import NumUtils;
import UniversalMemoryPool;

#pragma region EnumType

export enum EMEventType : uint8_t
{
	None = 0,
	ServerStart,
	ServerStop,
	ServerPause,
	ServerResume,
	AppStart,
	InitHotReload,
	DeinitHotReload,
	MovedDeinitHotReload,
};

export enum class EMComponentType : uint8_t
{
	None,
	ServerProxy,
	ServerEntityManager,
	ClientProxy,
	WebProxy,
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
	L10nText,
	Server,
	HotReload,
	Timer,
};


#pragma endregion

class Object;
class Component;
class Entity;
class System;
class World;
class Event;

export struct InstanceHolder
{
	using Ptr = std::shared_ptr<InstanceHolder>;
	InstanceHolder()
	{
		MemPool = std::make_shared<UniversalMemoryPool>();
	}

	struct MemPoolContext
	{
		std::source_location oLocation;
		UniversalMemoryPool::Ptr pMemPool;

		template<typename T, typename... Args>
		std::shared_ptr<T> Allocate(Args&&... args)
		{
			std::shared_ptr<T> object = pMemPool->Allocate<T>(std::forward<Args>(args)...);
			if (object)
			{
				pMemPool->SetMemoryRecordInfo(object.get(), std::move(oLocation));
			}
			return object;
		}
	};

	void Unload();

	UniversalMemoryPool::Ptr MemPool;

	std::shared_ptr<World> AuthWorld;

	std::shared_ptr<World> MainWorld;

	MemPoolContext GetMemPool(const std::source_location& location = std::source_location::current())
	{
		MemPoolContext context;
		context.oLocation = std::move(location);
		context.pMemPool = MemPool;

		return context;
	}

};

export std::shared_ptr<InstanceHolder> P_InstanceHolder;

#pragma region Event

export class Event
{
public:

	template<typename T, typename... Args>
	void AddEvent(EMEventType type, std::weak_ptr<T> entity, void (T::*callback)(Args...))
	{
		using FuncProxy = std::function<void(Args...)>;
		
		size_t objId = entity.lock()->ID();

		FuncProxy lumbdaFunc = [entity, callback](Args&&... args)
			{
				if (auto origin = entity.lock())
				{
					(origin.get()->*callback)(std::forward<Args>(args)...);
				}
			};

		mEventIdMap[objId][type] = lumbdaFunc;

		mEventCollection[type][objId] = objId;
	}

	template<typename... Args>
	void Broadcast(EMEventType type, Args&&... args)
	{
		using FuncProxy = std::function<void(Args...)>;

		for (auto& [objId, _] : mEventCollection[type])
		{
			auto& anyObj = mEventIdMap[objId][type];
			if(FuncProxy* typedFunc = std::any_cast<FuncProxy>(&anyObj))
			{
				(*typedFunc)(std::forward<Args>(args)...);
			}
		}
	}

	void MoveEvent(EMEventType origin, EMEventType target)
	{
		mEventCollection[target] = std::move(mEventCollection[origin]);
		for(auto& [_, objId] : mEventCollection[target])
		{
			auto& map = mEventIdMap[objId];
			map[target] = std::move(map[origin]);
			map.erase(origin);
		}

		mEventCollection.erase(origin);
	}

	void RemoveEvent(EMEventType origin)
	{
		auto map = std::move(mEventCollection[origin]);
		mEventCollection.erase(origin);

		for(auto& [_, objId] : map)
		{
			mEventIdMap[objId].erase(origin);
		}
		
	}

	void RemoveEvent(size_t objId)
	{
		auto it = mEventIdMap.find(objId);
		if (it != mEventIdMap.end())
		{
			for (auto& [type, _] : it->second)
			{
				mEventCollection[type].erase(objId);
			}

			mEventIdMap.erase(it);
		}
	}

private:
	std::unordered_map<size_t, std::unordered_map<EMEventType, std::any > > mEventIdMap;
	std::unordered_map<EMEventType, std::unordered_map<size_t, size_t>> mEventCollection;
};

#pragma endregion

#pragma region Object

export class Object : public std::enable_shared_from_this<Object>
{
public:
	using Ptr = std::shared_ptr<Object>;

	Object() = default;

	virtual ~Object()
	{
		if (!bIsDisposed)
		{
			// throw std::runtime_error("Object not disposed! Please check code!");
			std::cerr << "Object not disposed! Please check code!\n" << Platform::GetStackTrace() << "\n";
			if (P_InstanceHolder->MemPool)
			{
				std::cerr << "Object not disposed! Please check code!\n" << P_InstanceHolder->MemPool->GetMemoryRecordInfo(this) << "\n";
			}
		}
	}

public: // dll override

	size_t ID() { return iId; }

	void SetID(size_t id) { iId = id; }

	virtual void Dispose()
	{
		bIsDisposed = true;
	}

	virtual bool Awake() { return true; }

	template<typename T>
	std::shared_ptr<T> GetSelf() { return std::static_pointer_cast<T>(shared_from_this()); }

	template<typename T>
	std::weak_ptr<T> GetSelfW() { return GetSelf<T>(); }

	bool IsDisposed() { return bIsDisposed; }

protected:

	size_t iId = SFIdGenerator.nextId();

	bool bIsDisposed = false;
};

#pragma endregion

#pragma region Component


export class Component : public Object
{
protected:
	Component(std::weak_ptr<Entity> owner) :
		pOwner(owner)
	{

	}
public:
	using Ptr = std::shared_ptr<Component>;

	/// @brief this component owner
	virtual ~Component()
	{
	}

	virtual void Dispose() override;

	std::shared_ptr<Entity> GetOwner() { return pOwner.expired() ? nullptr : pOwner.lock(); }

	template<typename T>
	std::shared_ptr<T> GetOwner() { return std::static_pointer_cast<T>(GetOwner()); }

	std::shared_ptr<World> GetWorld();

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
	Entity(std::weak_ptr<World> world) :
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

	std::shared_ptr<World> GetWorld() { return pWorld.expired() ? nullptr : pWorld.lock(); }

	std::weak_ptr<World> GetWorldW() { return pWorld; }

	template<typename T>
	std::shared_ptr<T> GetComponent(EMComponentType type)
	{
		// this while lock when dispose***
		// std::unique_lock ulock(mComponentLock);

		auto it = mComponents.find(type);
		if ((!it->second->IsDisposed()); it != mComponents.end())
		{
			return std::static_pointer_cast<T>(it->second);
		}

		return nullptr;
	}

	virtual void Dispose() override;

	template<typename T>
	std::shared_ptr<T> AddComponent(const std::source_location& location = std::source_location::current())
	{
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from component");
		try
		{
			std::shared_ptr<T> component = P_InstanceHolder->GetMemPool(location).Allocate<T>(shared_from_this());
			if (!component->Awake())
			{
				component->Dispose();
				return nullptr;
			}
			std::unique_lock ulock(mComponentLock);
			mComponents.emplace(component->GetComponentType(), component);
			return component;
		}
		catch (const std::exception& e)
		{
			std::cerr << e.what() << '\n';
		}

		return nullptr;
	}

	void RemoveComponent(EMComponentType type)
	{
		std::unique_lock ulock(mComponentLock);

		auto it = mComponents.find(type);
		if (it == mComponents.end())
		{
			return;
		}

		mComponents.erase(type);
	}

protected: // dll proxy

	EMEntityType eEntityType = EMEntityType::None;

	std::weak_ptr<World> pWorld;

	std::unordered_map<EMComponentType, std::shared_ptr<Component>> mComponents;

	std::shared_mutex mComponentLock;
};

#pragma endregion

#pragma region System

export class System : public Entity
{
protected:

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
	std::shared_ptr<T> AddComponent(const std::source_location& location = std::source_location::current())
	{
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from component");
		try
		{
			std::shared_ptr<T> component = P_InstanceHolder->GetMemPool(location).Allocate<T>(GetSelfW<System>());
			if (!component->Awake())
			{
				component->Dispose();
				return nullptr;
			}
			mComponents.emplace(component->GetComponentType(), component);
			return component;
		}
		catch (const std::exception& e)
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

export class World : public Object, public Event
{
public:
	using Ptr = std::shared_ptr<World>;
	using WPtr = std::weak_ptr<World>;

	World() = default;

	virtual ~World()
	{
	}

	void AddSystem(System::Ptr system)
	{
		mSystemMap.emplace(system->GetSystemType(), system);
	}

	template<typename T = System>
	std::shared_ptr<T> AddSystem(const std::source_location& location = std::source_location::current())
	{
		static_assert(std::is_base_of_v<System, T>, "T must inherit from System");
		try
		{
			std::shared_ptr<T> system = P_InstanceHolder->GetMemPool(location).Allocate<T>(GetSelfW<World>());
			if (!system->Awake())
			{
				system->Dispose();
				return nullptr;
			}

			mSystemMap.emplace(system->GetSystemType(), system);
			return system;
		}
		catch (const std::exception& e)
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
			if ((!it->second->IsDisposed()); it != mSystemMap.end())
			{
				auto ptr = std::static_pointer_cast<T>(it->second);
				return ptr ? ptr : std::weak_ptr<T>{};
			}
		}
		catch (const std::exception& e)
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
			if ((!it->second->IsDisposed()); it != mSystemMap.end())
			{
				return std::static_pointer_cast<T>(it->second);
			}
		}
		catch (const std::exception& e)
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

	virtual void Dispose() override
	{
		Object::Dispose();

		if (mSystemMap.empty())
		{
			return;
		}

		auto it = mSystemMap.end();
		do
		{
			--it;
			it->second->Dispose();
		} while (it != mSystemMap.begin());

		mSystemMap.clear();
	}

	void MoveLuanchConfigToSelf(std::unordered_map<std::string, std::string>& config)
	{
		mLuanchConfig = std::move(config);
	}

	std::string* GetParam(const std::string& key)
	{
		if (mLuanchConfig.count(key))
		{
			return &mLuanchConfig[key];
		}

		return nullptr;
	}

	// single thread
	void PostTask(std::function<void()> task)
	{
		std::lock_guard<std::mutex> lock(oQueueMutex);
		mTasks.push(std::move(task));
	}

	virtual void TickMainFrame()
	{
		TickTask();
	}

protected:

	void TickTask()
	{
		std::lock_guard<std::mutex> lock(oQueueMutex);
		if (mTasks.empty())
		{
			return;
		}

		auto task = std::move(mTasks.front());
		mTasks.pop();

		task();
	}

private:

	std::unordered_map<std::string, std::string> mLuanchConfig;

	std::unordered_map<EMSystemType, std::shared_ptr<System>> mSystemMap;

	std::queue<std::function<void()>> mTasks;

	std::mutex oQueueMutex;

};

#pragma endregion

void InstanceHolder::Unload()
{
	if (MainWorld)
	{
		MainWorld->Dispose();
		MainWorld = nullptr;
	}

	if (AuthWorld)
	{
		AuthWorld->Dispose();
		AuthWorld = nullptr;
	}

	MemPool = nullptr;
}

std::shared_ptr<World> Component::GetWorld()
{
	auto owner = GetOwner();
	if (!owner)
	{
		return nullptr;
	}

	return owner->GetWorld();
}

void Component::Dispose()
{
	GetWorld()->RemoveEvent(ID());

	Object::Dispose();
}

void Entity::Dispose()
{
	GetWorld()->RemoveEvent(ID());
	
	Object::Dispose();

	if (mComponents.empty())
	{
		return;
	}

	std::unique_lock ulock(mComponentLock);

	auto it = mComponents.end();
	do
	{
		--it;
		it->second->Dispose();
		it = mComponents.erase(it);

	} while (it != mComponents.begin());

	mComponents.clear();
}
