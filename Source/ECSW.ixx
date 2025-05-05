module;
export module ECSW;

export import std.compat;

export enum class EMEntityType : uint8_t
{
	None,
	// NetEntity, virtual
	Server,
	Proxy,
	Client,
	Room,
};

export enum class EMComponentType : uint8_t
{
	None,
	DNServerProxy,
};

export enum class EMSystemType : uint8_t
{
	None,
	LoggerPrint,
	DNl10n,
	DNServer,
};

class World;

// normal data normal get/set
// if class std::function has logic. please imp to helper.

class ECSModle
{
public:
	ECSModle() = default;
	~ECSModle() = default;

	virtual bool Awake(){ return true;}

	virtual void Dispose()
	{
		bIsDisposed = true;
	}

	bool IsDispose()
	{
		return bIsDisposed;
	}

protected:

	bool bIsDisposed = false;
};

class Component : public ECSModle
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
		ECSModle::Dispose();

		pOwner = nullptr;
	}

	std::shared_ptr<Entity> GetOnwer(){ return pOwner; }
public:

	EMComponentType GetComponentType() { return eComponentType; }

protected: // dll proxy

	EMComponentType eComponentType = EMComponentType::None;

	std::shared_ptr<Entity> pOwner;
};


class Entity : public std::enable_shared_from_this<Entity>, public ECSModle
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
		ECSModle::Dispose();

		for (auto& component : mComponents)
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
			mComponents.emplace_back(component);
			return component;
		}
		catch(const std::exception& e)
		{
			std::cerr << e.what() << '\n';
		}
		
		return nullptr;
	}
	
protected: // dll proxy

	EMEntityType eEntityType = EMEntityType::None;

private:

	std::shared_ptr<World> pWorld;


	uint32_t iId = 0;

	std::vector<std::shared_ptr<Component>> mComponents;
};



class System : public Entity
{
    friend class World;
public:
	using Ptr = std::shared_ptr<System>;

	virtual ~System()
	{
	}

	void Dispose()
	{
		ECSModle::Dispose();
	}

	EMSystemType GetSystemType() { return eSystemType; }

protected:
	
	System(std::shared_ptr<World> world) 
		: Entity(world)
	{
	}

	EMSystemType eSystemType = EMSystemType::None;
};

class World : public std::enable_shared_from_this<World>
{
public:
	using Ptr = std::shared_ptr<World>;

	virtual ~World() = default;

	template<typename T = System>
	std::shared_ptr<T> AddSystem();

	template<typename T = System>
	std::shared_ptr<T> GetSystem(EMSystemType type);

	void Dispose();

	void MoveLuanchConfigToSelf(std::unordered_map<std::string, std::string>&& config)
	{
		mLuanchConfig = std::move(config);
	}

	std::string* LuanchParam(const std::string& key)
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

export 
{
	class Entity;
	class Component;
	class World;
	class System;
}

// void Entity::Dispose()
// {
// 	ECSModle::Dispose();

// 	for (auto& component : mComponents)
// 	{
// 		component->Dispose();
// 	}

// 	mComponents.clear();
// }

void World::Dispose()
{
	for (auto& system : mSystemMap)
	{
		system->Dispose();
	}
	mSystemMap.clear();
}

template<typename T>
std::shared_ptr<T> World::AddSystem()
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

template<typename T>
std::shared_ptr<T> World::GetSystem(EMSystemType type)
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
