module;
#include <functional> 
#include "hv/EventLoop.h"

#include "StdAfx.h"
export module DNServer;

import I10nText;

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

export struct MainPostMsg
{
	enum PostMsgType
	{
		None,
		Function,
		Command,
	};

	PostMsgType type;

	function<void()> pFunc;
	stringstream sCommand;

	MainPostMsg(){}

	MainPostMsg(const MainPostMsg& msg)
	{
		type = msg.type;
		switch(type)
		{
			case Command:
			sCommand << msg.sCommand.str();
			break;
			case Function:
            pFunc = msg.pFunc; // 拷贝函数对象
        	break;
		}
	}

};

export class DNServer
{
public:
	DNServer();
	virtual ~DNServer(){};

public:

	virtual bool Init();

	virtual void InitCmd(map<string, function<void(stringstream*)>> &cmdMap){ pCmdMap = &cmdMap; }

	virtual bool Start() = 0;

	virtual bool Stop() = 0;

	virtual void Pause() = 0;

	virtual void Resume() = 0;

    ServerType GetServerType(){return emServerType;}

	uint32_t &ServerIndex(){ return iServerIndex;}

	virtual void LoopEvent(function<void(hv::EventLoopPtr)> func){}

	bool& IsRun(){ return bInRun;}

	void TickMainFrame();

	void AddMsgTask(const MainPostMsg& postMsg);

public: // dll override
	DNl10n* pDNl10nInstance;
	map<string, string>* pLuanchConfig;

protected:
    ServerType emServerType;

	bool bInRun;

	uint32_t iServerIndex;

	list<MainPostMsg> mMessageTasks;

    mutex oTaskMutex;

	map<string, function<void(stringstream*)>>* pCmdMap;
};

DNServer::DNServer()
{
	emServerType = ServerType::None;
	bInRun = false;
	iServerIndex = 0;
	mMessageTasks.clear();
	pCmdMap = nullptr;
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
			for(MainPostMsg& postMsg : mMessageTasks)
			{
				switch(postMsg.type)
				{
					case MainPostMsg::Function:
						postMsg.pFunc();
					break;
					case MainPostMsg::Command:
					{
						string token;
						postMsg.sCommand >> token;
						if(pCmdMap && pCmdMap->contains(token))
						{
							(*pCmdMap)[token](&postMsg.sCommand);
						}
					}
					break;
				}
			}

			mMessageTasks.clear();
		}
	}
	
}

void DNServer::AddMsgTask(const MainPostMsg& postMsg)
{
	std::unique_lock<std::mutex> lock(oTaskMutex);
	mMessageTasks.push_back(postMsg);
}
