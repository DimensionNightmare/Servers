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

	bool& IsRun(){ return bInRun;}

	void TickMainFrame();

	void AddMsgTask(function<void()> func);

public: // dll override
	DNl10n* pDNl10nInstance;
	map<string, string>* pLuanchConfig;

protected:
    ServerType emServerType;

	bool bInRun;
	unsigned int iServerIndex;

	list<function<void()>> mMessageTasks;
    mutex oTaskMutex;
};

DNServer::DNServer()
{
	emServerType = ServerType::None;
	bInRun = false;
	iServerIndex = 0;

	mMessageTasks.clear();
}

bool DNServer::Init()
{
	string* value = GetLuanchConfigParam("svrIndex");
	if(value)
	{
		iServerIndex = stoi(*value);
	}

	return true;
}

void DNServer::TickMainFrame()
{
	// mMessageTasks
	{
		unique_lock<mutex> lock(oTaskMutex);
		if(mMessageTasks.size())
		{
			for(function<void()>& func : mMessageTasks)
			{
				func();
			}

			mMessageTasks.clear();
		}
	}
	
}

void DNServer::AddMsgTask(function<void()> func)
{
	std::unique_lock<std::mutex> lock(oTaskMutex);
	mMessageTasks.push_back(func);
}
