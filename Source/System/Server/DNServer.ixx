module;
export module DNServer;

import ECSW;
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

	
protected:
	friend class World;
	DNServer(World::WPtr world):System(world)
	{
		emSystemType = EMSystemType::DNServer;

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

	uint32_t& ServerId() { return iServerId; }

public: // dll override

protected:

	EMServerType emServerType = EMServerType::None;

	uint32_t iServerId = 0;

	std::mutex oTaskMutex;
};
