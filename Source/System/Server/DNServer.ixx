module;
export module DNServer;

import L10nText;
import Config.Server;
import ThirdParty.Libhv;
import std.compat;
import ECSW;

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
	friend class World;
	DNServer(World::Ptr world):System(world)
	{
		eSystemType = EMSystemType::DNServer;

		Libhv::hvlog_disable();
	}
public:
	using Ptr = std::shared_ptr<DNServer>;

	virtual ~DNServer()
	{
	}

	virtual bool Awake() override
	{
		return Init();
	}

	virtual bool Init()
	{
		if (std::string* value = GetWorld()->LuanchParam("svrIndex"))
		{
			iServerId = stoi(*value);
		}

		return true;
	}

	virtual void InitCmd( std::unordered_map<std::string, std::function<void(std::stringstream*)>>& cmdMap) 
	{ 
		pCmdMap = &cmdMap; 
	}

	virtual bool Start(){ return true;}

	virtual bool Stop(){ return true;}

	virtual void Pause(){}

	virtual void Resume(){}

	EMServerType GetServerType() { return emServerType; }

	uint32_t& ServerId() { return iServerId; }

	virtual void LoopEvent(std::function<void(hv::EventLoopPtr)> func){}

	virtual void TickMainFrame(){}

public: // dll override

protected:

	EMServerType emServerType = EMServerType::None;

	uint32_t iServerId = 0;

	std::mutex oTaskMutex;

	std::unordered_map<std::string, std::function<void(std::stringstream*)>>* pCmdMap = nullptr;
};
