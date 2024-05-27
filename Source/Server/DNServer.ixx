module;
#include <functional> 
#include <cstdint>
#include <list>
#include "hv/EventLoop.h"

#include "StdMacro.h"
export module DNServer;

import I10nText;
import Config.Server;

using namespace std;

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

	virtual void InitCmd(map<string, function<void(stringstream*)>>& cmdMap) { pCmdMap = &cmdMap; }

	virtual bool Start() = 0;

	virtual bool Stop() = 0;

	virtual void Pause() = 0;

	virtual void Resume() = 0;

	ServerType GetServerType() { return emServerType; }

	uint32_t& ServerIndex() { return iServerIndex; }

	virtual void LoopEvent(function<void(hv::EventLoopPtr)> func) {}

	bool& IsRun() { return bInRun; }

	void TickMainFrame();

public: // dll override
	DNl10n* pDNl10nInstance;
	map<string, string>* pLuanchConfig;

protected:
	ServerType emServerType;

	bool bInRun;

	uint32_t iServerIndex;

	mutex oTaskMutex;

	map<string, function<void(stringstream*)>>* pCmdMap;
};

DNServer::DNServer()
{
	emServerType = ServerType::None;
	pDNl10nInstance = nullptr;
	pLuanchConfig = nullptr;
	bInRun = false;
	iServerIndex = 0;
	pCmdMap = nullptr;
}

DNServer::~DNServer()
{
}

bool DNServer::Init()
{
	string* value = GetLuanchConfigParam("svrIndex");
	if (value)
	{
		iServerIndex = stoi(*value);
	}

	return true;
}

void DNServer::TickMainFrame()
{
	
}
