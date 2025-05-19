module;
export module DNServer;

import ECSW;
import ThirdParty.Libhv;
import Logger;

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

export using ServerTypeBitFlag = std::bitset<static_cast<uint8_t>(EMServerType::Max)>;

export std::array<std::pair<uint8_t, std::string>, 7> ServerTypeList = {{
	#define one(name) {static_cast<uint8_t>(EMServerType::name), #name}
	one(ControlServer),
	one(GlobalServer),
	one(AuthServer),
	one(GateServer),
	one(DatabaseServer),
	one(LogicServer),
	one(DedicatedServer),
	#undef one
}};


export class DNServer : public System
{
public:
	using Ptr = std::shared_ptr<DNServer>;
	using WPtr = std::weak_ptr<DNServer>;
	
protected:
	friend class World;
	DNServer(World::WPtr world):System(world)
	{
		emSystemType = EMSystemType::DNServer;

		pLogger = GetWorld()->GetSystemW<LoggerPrint>(EMSystemType::LoggerPrint);

		Libhv::hvlog_disable();
	}
public:
	

	virtual ~DNServer()
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
			iServerId = stoi(*value);
		}

		return true;
	}

	EMServerType GetServerType() { return emServerType; }
	void SetServerType(EMServerType type) { emServerType = type; }

	uint64_t ServerId() { return iServerId; }
	void SetServerId(uint64_t id) { iServerId = id; }

	LoggerPrint::Ptr GetLogger() { return pLogger.expired() ? nullptr : pLogger.lock(); }
public: // dll override

protected:

	EMServerType emServerType = EMServerType::None;

	uint64_t iServerId = 0;

	std::mutex oTaskMutex;

	LoggerPrint::WPtr pLogger;
};
