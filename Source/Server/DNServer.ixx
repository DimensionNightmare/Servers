module;
export module DNServer;

import L10nText;
import Config.Server;
import ThirdParty.Libhv;
import std.compat;

export enum class EMServerType : uint8_t
{
	None,
	ControlServer,
	GlobalServer,
	AuthServer,

	GateServer,
	DatabaseServer,
	LogicServer,

	DedicatedServer,

	Max,
};

export class DNServer
{

public:

	DNServer()
	{
	}

	virtual ~DNServer()
	{
	}

	virtual bool Init()
	{
		std::string* value = LaunchConfig::GetParam("svrIndex");
		if (value)
		{
			iServerId = stoi(*value);
		}

		return true;
	}

	virtual void InitCmd( std::unordered_map<std::string, std::function<void(std::stringstream*)>>& cmdMap) 
	{ 
		pCmdMap = &cmdMap; 
	}

	virtual bool Start() = 0;

	virtual bool Stop() = 0;

	virtual void Pause() = 0;

	virtual void Resume() = 0;

	EMServerType GetServerType() { return emServerType; }

	uint32_t& ServerId() { return iServerId; }

	virtual void LoopEvent(std::function<void(hv::EventLoopPtr)> func) = 0;

	virtual void TickMainFrame(){}

public: // dll override

protected:

	EMServerType emServerType = EMServerType::None;

	uint32_t iServerId = 0;

	std::mutex oTaskMutex;

	std::unordered_map<std::string, std::function<void(std::stringstream*)>>* pCmdMap = nullptr;
};
