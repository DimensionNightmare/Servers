module;
#include "StdAfx.h"
#include "hv/EventLoop.h"

#include <functional> 
export module DNServer;

using namespace std;

export enum class ServerType : unsigned char
{
    None,
    ControlServer,
    GlobalServer,
	AuthServer,

	GateServer,
	DatabaseServer,
	DedicatedServer,
	
	Max,
};

export class DNServer
{
public:
	DNServer();
	virtual ~DNServer(){};

public:

	virtual bool Init();

	virtual void InitCmd(map<string, function<void(stringstream*)>> &cmdMap) = 0;

	virtual bool Start() = 0;

	virtual bool Stop() = 0;

	virtual void Pause() = 0;

	virtual void Resume() = 0;

    ServerType GetServerType(){return emServerType;}

	unsigned int &ServerIndex(){ return iServerIndex;}

	virtual void LoopEvent(function<void(hv::EventLoopPtr)> func){}

	bool IsRun(){ return bInRun;}
	void SetRun(bool state){bInRun = state;}

public: // dll override
	DNl10n* pDNl10nInstance;
	map<string, string>* pLuanchConfig;

protected:
    ServerType emServerType;

	bool bInRun;
	unsigned int iServerIndex;

	DWORD oThreadId;
};

DNServer::DNServer()
{
	emServerType = ServerType::None;
	bInRun = false;
	iServerIndex = 0;

	oThreadId = 0;
}

bool DNServer::Init()
{
	string* value = GetLuanchConfigParam("svrIndex");
	if(value)
	{
		iServerIndex = stoi(*value);
	}

	oThreadId = GetCurrentThreadId();

	return true;
}
