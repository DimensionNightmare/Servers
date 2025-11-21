export module Server;

import Logger;
import ECSW;
import std.compat;

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
	Client			= 255,
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
	friend class UniversalMemoryPool;
	
	Server(World::CVPtr world):System(world)
	{
		emSystemType = EMSystemType::Server;
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
		if (std::string* value = GetWorld()->GetParam("svrIndex"))
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
