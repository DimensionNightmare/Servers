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

export  // enum class only. enum dont.
template<typename EnumT>
requires std::is_enum_v<EnumT>
class Event
{
public:

	template<auto Func>
	bool AddEvent(EnumT type, std::shared_ptr<typename FunctionTraits<decltype(Func)>::ClassType> entity)
	{
		if(!entity)
		{
			return false;
		}

		size_t objId = entity->ID();
		mEventIdMap[objId][type] = std::make_unique< EventContainer<Func> >(entity);
		mEventCollection[type][objId] = objId;

		return true;
	}

	void AddEvent(EnumT type, auto&& func)
	{
		using Traits = FunctionTraits<std::decay_t<decltype(func)>>;
		using FuncSign = typename Traits::FuncSign;

		mEventCollectionWithoutId[type].emplace_back(std::make_unique< DynamicEventContainer<FuncSign> >(&func));
	}

	template<auto Func>
	void AddEvent(EnumT type)
	{
		mEventCollectionWithoutId[type].emplace_back(std::make_unique< EventContainer<Func> >());
	}

	template<typename... Args>
	void Broadcast(EnumT type, Args&&... args)
	{
		constexpr auto typeHash = TupleTypeHash<std::tuple<std::decay_t<Args>...>>();

		auto params = std::forward_as_tuple(std::forward<Args>(args)...);

		auto dealFunc = [&](IEventContainer* handle)
		{
			if(handle->mTypeHash == typeHash)
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

		{
			auto handles = mEventCollection[type]
				| std::views::keys
				| std::views::transform([&](const auto& param){
					return mEventIdMap[param][type].get();
				}); 

			for (const auto& handle : handles)
			{
				dealFunc(handle);
			}
		}
		
		{
			for (const auto& handle : mEventCollectionWithoutId[type])
			{
				dealFunc(handle.get());
			}
		}

	}

	void MoveEvent(EnumT origin, EnumT target)
	{

		{
			mEventCollection[target] = std::move(mEventCollection[origin]);
			
			auto objIds = mEventCollection[target]
				| std::views::values;

			for (const auto& objId : objIds)
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


	void RemoveEvent(EnumT origin)
	{
		{
			auto map = std::move(mEventCollection[origin]);
			mEventCollection.erase(origin);

			auto objIds = map 
				| std::views::values;

			for (const auto& objId : objIds)
			{
				mEventIdMap[objId].erase(origin);
			}
		}
		
		{
			mEventCollectionWithoutId.erase(origin);
		}
	}

	void RemoveEvent(size_t objId)
	{
		auto it = mEventIdMap.find(objId);
		if (it != mEventIdMap.end())
		{
			auto names = it->second 
				| std::views::keys;

			for (const auto& type : names)
			{
				mEventCollection[type].erase(objId);
			}

			mEventIdMap.erase(it);
		}
	}

protected:
	std::unordered_map<size_t, std::unordered_map<EnumT, std::unique_ptr<IEventContainer> > > mEventIdMap;
	std::unordered_map<EnumT, std::unordered_map<size_t, size_t>> mEventCollection;

	std::unordered_map<EnumT, std::list<std::unique_ptr<IEventContainer> >> mEventCollectionWithoutId;
};

#pragma endregion

#pragma region Object

export class Object : public std::enable_shared_from_this<Object>
{
public:
	using Ptr = std::shared_ptr<Object>;
	using CVPtr = const Ptr&;

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
				__debugbreak();
			}
		}
	}

public: 

	size_t ID() { return iId; }

	void SetID(size_t id) { iId = id; }

	virtual void Dispose()
	{
		bIsDisposed = true;

		int useCount = weak_from_this().use_count();

		if(useCount > 1)
		{
			std::cerr << "Object disposed Has Other Owner! Please check Other Obj!\n";
			__debugbreak();
		}
	}

	virtual bool Awake() { return true; }

	template<typename T>
	std::shared_ptr<T> GetSelf() { return std::static_pointer_cast<T>(shared_from_this()); }

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
	Component(const std::shared_ptr<Entity>& owner) :
		pOwner(owner)
	{

	}
public:
	using Ptr = std::shared_ptr<Component>;
	using CVPtr = const Ptr&;

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
	Entity(const std::shared_ptr<World>& world) :
		pWorld(world)
	{

	}

public:
	using Ptr = std::shared_ptr<Entity>;
	using CVPtr = const Ptr&;

	using WPtr = std::weak_ptr<Entity>;

	virtual ~Entity()
	{
	}

public: 

	/// @brief entity type total enum
	EMEntityType GetEntityType() { return eEntityType; }

	std::shared_ptr<World> GetWorld() { return pWorld.expired() ? nullptr : pWorld.lock(); }

	template<typename T>
	std::shared_ptr<T> GetComponent(EMComponentType type)
	{
		// this while lock when dispose***
		// std::unique_lock ulock(mComponentLock);

		auto it = mComponents.find(type);
		if(it != mComponents.end() && !it->second->IsDisposed())
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
			std::shared_ptr<T> component = P_InstanceHolder->GetMemPool(location).Allocate<T>(GetSelf<Entity>());
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

	System(const std::shared_ptr<World>& world)
		: Entity(world)
	{
	}


public:
	using Ptr = std::shared_ptr<System>;
	using CVPtr = const Ptr&;

	virtual ~System()
	{
	}

	EMSystemType GetSystemType() { return emSystemType; }

	template<typename T>
	std::shared_ptr<T> AddComponent(const std::source_location& location = std::source_location::current())
	{
		static_assert(std::is_base_of_v<Component, T>, "T must inherit from component");
		try
		{
			std::shared_ptr<T> component = P_InstanceHolder->GetMemPool(location).Allocate<T>(GetSelf<System>());
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

export class World : public Object, public Event<EMEventType>
{
public:
	using Ptr = std::shared_ptr<World>;
	using CVPtr = const Ptr&;
	using WPtr = std::weak_ptr<World>;

	World() = default;

	virtual ~World()
	{
	}
	

	template<typename T = System>
	std::shared_ptr<T> AddSystem(const std::source_location& location = std::source_location::current())
	{
		static_assert(std::is_base_of_v<System, T>, "T must inherit from System");
		try
		{
			std::shared_ptr<T> system = P_InstanceHolder->GetMemPool(location).Allocate<T>(GetSelf<World>());
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
	std::shared_ptr<T> GetSystem(EMSystemType type)
	{
		static_assert(std::is_base_of_v<System, T>, "T must inherit from System");
		try
		{
			auto it = mSystemMap.find(type);
			if(it != mSystemMap.end() && !it->second->IsDisposed())
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

	void RemoveSystem(EMSystemType type)
	{
		mSystemMap.erase(type);
	}

	virtual void Dispose() override
	{
		if (!mSystemMap.empty())
		{
			auto it = mSystemMap.end();
			do
			{
				--it;
				it->second->Dispose();
			} while (it != mSystemMap.begin());
			
			mSystemMap.clear();
		}

		Object::Dispose();
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

	void SetParam(std::string key, const std::string value)
	{
		mLuanchConfig[key] = value;
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
