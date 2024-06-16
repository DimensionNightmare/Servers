module;
#include <functional> 
#include <cstdint>
#include <list>
#include <string>
#include <mutex>

#include "StdMacro.h"
export module DNServer;

import I10nText;
import Config.Server;
import ThirdParty.Libhv;

export enum class ServerType : uint8_t
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
	DNServer();
	virtual ~DNServer();

public:

	virtual bool Init();

	virtual void InitCmd(unordered_map<string, function<void(stringstream*)>>& cmdMap) { pCmdMap = &cmdMap; }

	virtual bool Start() = 0;

	virtual bool Stop() = 0;

	virtual void Pause() = 0;

	virtual void Resume() = 0;

	ServerType GetServerType() { return emServerType; }

	uint32_t& ServerId() { return iServerId; }

	virtual void LoopEvent(function<void(EventLoopPtr)> func) = 0;

	bool& IsRun() { return bInRun; }

	virtual void TickMainFrame();

public: // dll override
	DNl10n* pDNl10nInstance = nullptr;

	unordered_map<string, string>* pLuanchConfig = nullptr;

protected:
	ServerType emServerType = ServerType::None;

	bool bInRun = false;

	uint32_t iServerId = 0;

	mutex oTaskMutex;

	unordered_map<string, function<void(stringstream*)>>* pCmdMap = nullptr;
};

DNServer::DNServer()
{
}

DNServer::~DNServer()
{
}

bool DNServer::Init()
{
	string* value = GetLuanchConfigParam("svrIndex");
	if (value)
	{
		iServerId = stoi(*value);
	}

	return true;
}

void DNServer::TickMainFrame()
{

}
