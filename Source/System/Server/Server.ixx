export module Server;

import Logger;
import ECSW;
import std.compat;
import ThirdParty.Libhv;

export enum class EMServerType : uint8_t
{
	None = 0			,
	ControlServer 		,
	GlobalServer 		,
	AuthServer 			,

	GateServer 			,
	DatabaseServer 		,
	LogicServer 		,

	DedicatedServer 	,
	Max					,
};

export std::array<std::pair<EMServerType, std::string>, 7> ServerTypeList = {{
	#define one(name) {EMServerType::name, #name}
	one(ControlServer),
	one(GlobalServer),
	one(AuthServer),
	one(GateServer),
	one(DatabaseServer),
	one(LogicServer),
	#undef one
}}; // dynamic initializer


export class Server : public System
{
public:
	using Ptr = std::shared_ptr<Server>;
	using CVPtr = const Ptr&;
	using WPtr = std::weak_ptr<Server>;
	
protected:
	friend class World;
	friend class UniversalMemoryPool;
	
	Server(World::WPtr world):System(world)
	{
		emSystemType = EMSystemType::Server;

		Libhv::hvlog_disable();
	}
public:
	

	virtual ~Server()
	{
	}

	virtual void Dispose() override
	{
		System::Dispose();
	}

	virtual bool Awake() override
	{
		if (std::string* value = GetWorld()->LaunchParam("svrIndex"))
		{
			bIsPull = true;
			SetID(stol(*value));
		}

		return true;
	}

	EMServerType GetServerType() { return emServerType; }
	void SetServerType(EMServerType type) { emServerType = type; }

	bool IsPullServer() { return bIsPull;}
	
public: // dll override

protected:

	EMServerType emServerType = EMServerType::None;

	bool bIsPull = false;

	std::mutex oTaskMutex;
};
