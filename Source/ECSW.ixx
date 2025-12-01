export module ECSW;

import std.compat;
import ThirdParty.Platform;
import NumUtils;
import UniversalMemoryPool;
import FuncUtils;
import StrUtils;

#pragma region EnumType

export enum class EMEventType : uint8_t
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
	InitedRdbConnection,
	ClientProxyRegist,
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

export struct InstanceHolder
{
	using Ptr = std::shared_ptr<InstanceHolder>;
	using CVPtr = const Ptr&;

	InstanceHolder()
	{
		MemPool = std::make_shared<UniversalMemoryPool>();
	}

	/// @brief Context for memory pool allocation with source location tracking
	struct MemPoolContext
	{
		std::source_location oLocation;
		UniversalMemoryPool::Ptr pMemPool;

		template<typename T, typename... Args>
		[[nodiscard]] std::shared_ptr<T> Allocate(Args&&... args)
		{
			auto object = pMemPool->Allocate<T>(std::forward<Args>(args)...);
			if (object) [[likely]]
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

	[[nodiscard]] MemPoolContext GetMemPool(const std::source_location& location = std::source_location::current()) noexcept
	{
		return MemPoolContext{
			.oLocation = location,
			.pMemPool = MemPool
		};
	}
};

export std::shared_ptr<InstanceHolder> P_InstanceHolder;

#pragma region Event

/// @brief Type-safe event system using enum types
export template<typename EnumT>
requires std::is_enum_v<EnumT>
class Event
{
public:
	/// @brief Register a member function as event handler
	template<auto Func>
	bool AddEvent(EnumT type, std::shared_ptr<typename FunctionTraits<decltype(Func)>::ClassType> entity)
	{
		if (!entity) [[unlikely]]
		{
			return false;
		}

		size_t objId = entity->ID();
		mEventIdMap[objId][type] = std::make_unique<EventContainer<Func>>(entity);
		mEventCollection[type][objId] = objId;

		return true;
	}

	/// @brief Register a callable as event handler
	void AddEvent(EnumT type, auto&& func)
	{
		using Traits = FunctionTraits<std::decay_t<decltype(func)>>;
		using FuncSign = typename Traits::FuncSign;

		mEventCollectionWithoutId[type].emplace_back(
			std::make_unique<DynamicEventContainer<FuncSign>>(&func)
		);
	}

	/// @brief Register a static function as event handler
	template<auto Func>
	void AddEvent(EnumT type)
	{
		mEventCollectionWithoutId[type].emplace_back(
			std::make_unique<EventContainer<Func>>()
		);
	}

	/// @brief Broadcast event to all registered handlers
	template<typename... Args>
	void Broadcast(EnumT type, Args&&... args)
	{
		constexpr auto typeHash = TupleTypeHash<std::tuple<std::decay_t<Args>...>>();

		auto params = std::forward_as_tuple(std::forward<Args>(args)...);

		auto dealFunc = [&](IEventContainer* handle)
		{
			if (handle->mTypeHash == typeHash) [[likely]]
			{
				handle->Invoke(&params);
			}
			else
			{
#if DN_DEBUG_EVENT
				constexpr auto typeSign = TupleTypeStr<std::tuple<std::decay_t<Args>...>>();
				std::cerr << "Event Sign Not Match!! -> " << EnumName(type) << "\n" 
					<< handle->sTypeSign << "\n"
					<< typeSign << "\n\n";
#else
				std::cerr << "Event Sign Not Match!! -> " << EnumName(type) << "\n\n";
#endif
			}
		};

		// Process object-bound events
		{
			auto handles = mEventCollection[type]
				| std::views::keys
				| std::views::transform([this, type](const auto& param) {
					return mEventIdMap[param][type].get();
				}); 

			for (const auto& handle : handles)
			{
				dealFunc(handle);
			}
		}
		
		// Process standalone events
		{
			for (const auto& handle : mEventCollectionWithoutId[type])
			{
				dealFunc(handle.get());
			}
		}
	}

	/// @brief Move events from one type to another
	void MoveEvent(EnumT origin, EnumT target)
	{
		{
			mEventCollection[target] = std::move(mEventCollection[origin]);
			
			for (const auto& objId : mEventCollection[target] | std::views::values)
			{
				auto& map = mEventIdMap[objId];
				map[target] = std::move(map[origin]);
				map.erase(origin);
			}

			mEventCollection.erase(origin);
		}
		
		{
			mEventCollectionWithoutId[target] = std::move(mEventCollectionWithoutId[origin]);
		}
	}

	/// @brief Remove all events of a type
	void RemoveEvent(EnumT origin)
	{
		{
			auto map = std::move(mEventCollection[origin]);
			mEventCollection.erase(origin);

			for (const auto& objId : map | std::views::values)
			{
				mEventIdMap[objId].erase(origin);
			}
		}
		
		{
			mEventCollectionWithoutId.erase(origin);
		}
	}

	/// @brief Remove all events for an object
	void RemoveEvent(size_t objId)
	{
		if (auto it = mEventIdMap.find(objId); it != mEventIdMap.end())
		{
			for (const auto& type : it->second | std::views::keys)
			{
				mEventCollection[type].erase(objId);
			}

			mEventIdMap.erase(it);
		}
	}

protected:
	std::unordered_map<size_t, std::unordered_map<EnumT, std::unique_ptr<IEventContainer>>> mEventIdMap;
	std::unordered_map<EnumT, std::unordered_map<size_t, size_t>> mEventCollection;
	std::unordered_map<EnumT, std::list<std::unique_ptr<IEventContainer>>> mEventCollectionWithoutId;
};

#pragma endregion

#pragma region Object

/// @brief Base object class with automatic disposal tracking
export class Object : public std::enable_shared_from_this<Object>
{
public:
	using Ptr = std::shared_ptr<Object>;
	using CVPtr = const Ptr&;

	Object() = default;
	Object(const Object&) = delete;
	Object& operator=(const Object&) = delete;
	Object(Object&&) = delete;
	Object& operator=(Object&&) = delete;

	virtual ~Object()
	{
		if (!bIsDisposed) [[unlikely]]
		{
			std::cerr << "Object not disposed! Please check code!\n" << Platform::GetStackTrace() << "\n";
			if (P_InstanceHolder && P_InstanceHolder->MemPool)
			{
				std::cerr << "Object not disposed! Please check code!\n" 
					<< P_InstanceHolder->MemPool->GetMemoryRecordInfo(this) << "\n";
				__debugbreak();
			}
		}
	}

	[[nodiscard]] size_t ID() const noexcept { return iId; }

	void SetID(size_t id) noexcept { iId = id; }

	virtual void Dispose()
	{
		bIsDisposed = true;

		if (int useCount = weak_from_this().use_count(); useCount > 1) [[unlikely]]
		{
			std::cerr << "Object disposed Has Other Owner! Please check Other Obj!\n";
			__debugbreak();
		}
	}

	virtual bool Awake() { return true; }

	template<typename T>
	[[nodiscard]] std::shared_ptr<T> GetSelf() 
	{ 
		return std::static_pointer_cast<T>(shared_from_this()); 
	}

	[[nodiscard]] bool IsDisposed() const noexcept { return bIsDisposed; }

protected:
	size_t iId = SFIdGenerator.nextId();
	bool bIsDisposed = false;
};

#pragma endregion

#pragma region Component


/// @brief Component base class attached to entities
export class Component : public Object
{
protected:
	explicit Component(const std::shared_ptr<Entity>& owner) noexcept
		: pOwner(owner)
	{
	}

public:
	using Ptr = std::shared_ptr<Component>;
	using CVPtr = const Ptr&;

	~Component() override = default;

	void Dispose() override;

	[[nodiscard]] std::shared_ptr<Entity> GetOwner() noexcept 
	{ 
		return pOwner.expired() ? nullptr : pOwner.lock(); 
	}

	template<typename T>
	[[nodiscard]] std::shared_ptr<T> GetOwner() noexcept 
	{ 
		return std::static_pointer_cast<T>(GetOwner()); 
	}

	[[nodiscard]] std::shared_ptr<World> GetWorld();

	[[nodiscard]] EMComponentType GetComponentType() const noexcept { return eComponentType; }

protected:
	EMComponentType eComponentType = EMComponentType::None;
	std::weak_ptr<Entity> pOwner;
};

#pragma endregion

#pragma region Entity

/// @brief Entity base class with component management
export class Entity : public Object
{
protected:
	explicit Entity(const std::shared_ptr<World>& world) noexcept
		: pWorld(world)
	{
	}

public:
	using Ptr = std::shared_ptr<Entity>;
	using CVPtr = const Ptr&;
	using WPtr = std::weak_ptr<Entity>;

	~Entity() override = default;

	[[nodiscard]] EMEntityType GetEntityType() const noexcept { return eEntityType; }

	[[nodiscard]] std::shared_ptr<World> GetWorld() noexcept 
	{ 
		return pWorld.expired() ? nullptr : pWorld.lock(); 
	}

	template<typename T>
	[[nodiscard]] std::shared_ptr<T> GetComponent(EMComponentType type) noexcept
	{
		if (auto it = mComponents.find(type); it != mComponents.end() && !it->second->IsDisposed())
		{
			return std::static_pointer_cast<T>(it->second);
		}
		return nullptr;
	}

	void Dispose() override;

	template<typename T>
	[[nodiscard]] std::shared_ptr<T> AddComponent(const std::source_location& location = std::source_location::current())
	{
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");
		try
		{
			auto component = P_InstanceHolder->GetMemPool(location).Allocate<T>(GetSelf<Entity>());
			if (!component->Awake()) [[unlikely]]
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
		mComponents.erase(type);
	}

protected:
	EMEntityType eEntityType = EMEntityType::None;
	std::weak_ptr<World> pWorld;
	std::unordered_map<EMComponentType, std::shared_ptr<Component>> mComponents;
	std::shared_mutex mComponentLock;
};

#pragma endregion

#pragma region System

/// @brief System entity with specialized component management
export class System : public Entity
{
protected:
	explicit System(const std::shared_ptr<World>& world) noexcept
		: Entity(world)
	{
	}

public:
	using Ptr = std::shared_ptr<System>;
	using CVPtr = const Ptr&;

	~System() override = default;

	[[nodiscard]] EMSystemType GetSystemType() const noexcept { return emSystemType; }

	template<typename T>
	[[nodiscard]] std::shared_ptr<T> AddComponent(const std::source_location& location = std::source_location::current())
	{
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");
		try
		{
			auto component = P_InstanceHolder->GetMemPool(location).Allocate<T>(GetSelf<System>());
			if (!component->Awake()) [[unlikely]]
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

/// @brief World class managing systems and events
export class World : public Object, public Event<EMEventType>
{
public:
	using Ptr = std::shared_ptr<World>;
	using CVPtr = const Ptr&;
	using WPtr = std::weak_ptr<World>;

	World() = default;
	~World() override = default;

	template<typename T = System>
	[[nodiscard]] std::shared_ptr<T> AddSystem(const std::source_location& location = std::source_location::current())
	{
		static_assert(std::is_base_of_v<System, T>, "T must inherit from System");
		try
		{
			auto system = P_InstanceHolder->GetMemPool(location).Allocate<T>(GetSelf<World>());
			if (!system->Awake()) [[unlikely]]
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
	[[nodiscard]] std::shared_ptr<T> GetSystem(EMSystemType type) noexcept
	{
		static_assert(std::is_base_of_v<System, T>, "T must inherit from System");
		if (auto it = mSystemMap.find(type); it != mSystemMap.end() && !it->second->IsDisposed())
		{
			return std::static_pointer_cast<T>(it->second);
		}
		return nullptr;
	}

	void RemoveSystem(EMSystemType type)
	{
		mSystemMap.erase(type);
	}

	void Dispose() override
	{
		if (!mSystemMap.empty())
		{
			for (auto it = mSystemMap.rbegin(); it != mSystemMap.rend(); ++it)
			{
				it->second->Dispose();
			}
			mSystemMap.clear();
		}

		Object::Dispose();
	}

	void MoveLuanchConfigToSelf(std::unordered_map<std::string, std::string>& config) noexcept
	{
		mLuanchConfig = std::move(config);
	}

	[[nodiscard]] std::string* GetParam(const std::string& key) noexcept
	{
		if (auto it = mLuanchConfig.find(key); it != mLuanchConfig.end())
		{
			return &it->second;
		}
		return nullptr;
	}

	void SetParam(std::string key, std::string value)
	{
		mLuanchConfig[std::move(key)] = std::move(value);
	}

	void PostTask(std::function<void()> task)
	{
		std::lock_guard lock(oQueueMutex);
		mTasks.push(std::move(task));
	}

	virtual void TickMainFrame()
	{
		TickTask();
	}

protected:
	void TickTask()
	{
		std::lock_guard lock(oQueueMutex);
		if (mTasks.empty()) [[likely]]
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
	if (auto owner = GetOwner())
	{
		return owner->GetWorld();
	}
	
	return nullptr;
}

void Component::Dispose()
{
	GetWorld()->RemoveEvent(ID());

	Object::Dispose();
}

void Entity::Dispose()
{
	GetWorld()->RemoveEvent(ID());
	
	if (!mComponents.empty())
	{
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

	Object::Dispose();
}
