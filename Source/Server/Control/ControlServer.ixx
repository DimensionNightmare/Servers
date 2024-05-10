module;
#include "hv/EventLoop.h"
#include "hv/EventLoopThread.h"

#include "StdAfx.h"
export module ControlServer;

export import DNServer;
import DNServerProxy;
import MessagePack;
import ServerEntityManager;

using namespace std;
using namespace hv;

export class ControlServer : public DNServer
{
public:
	ControlServer();

	~ControlServer();

	virtual bool Init() override;

	virtual void InitCmd(map<string, function<void(stringstream*)>> &cmdMap) override;

	virtual bool Start() override;

	virtual bool Stop() override;

	virtual void Pause() override;

	virtual void Resume() override;

	virtual void LoopEvent(function<void(EventLoopPtr)> func) override;

public: // dll override
	virtual DNServerProxy* GetSSock(){return pSSock;}
	virtual ServerEntityManager* GetServerEntityManager(){return pServerEntityMan;}

protected: // dll proxy
	DNServerProxy* pSSock;

	ServerEntityManager* pServerEntityMan;
};



ControlServer::ControlServer()
{
	emServerType = ServerType::ControlServer;
	pSSock = nullptr;
	pServerEntityMan = nullptr;
}

ControlServer::~ControlServer()
{
	delete pServerEntityMan;
	pServerEntityMan = nullptr;

	if(pSSock)
	{
		delete pSSock;
		pSSock = nullptr;
	}
}

bool ControlServer::Init()
{
	string* ip = GetLuanchConfigParam("ip");
	string* port = GetLuanchConfigParam("port");
	if(!ip || !port)
	{
		DNPrint(ErrCode_SrvNeedIPPort, LoggerLevel::Error, nullptr);
		return false;
	}

	DNServer::Init();

	pSSock = new DNServerProxy;
	pSSock->pLoop = make_shared<EventLoopThread>();

	int listenfd = pSSock->createsocket(stoi(*port));
	if (listenfd < 0)
	{
		DNPrint(ErrCode_CreateSocket, LoggerLevel::Error, nullptr);
		return false;
	}

	DNPrint(TipCode_SrvListenOn, LoggerLevel::Normal, nullptr, pSSock->port, listenfd);

	unpack_setting_t* setting = new unpack_setting_t;
	setting->mode = unpack_mode_e::UNPACK_BY_LENGTH_FIELD;
	setting->length_field_coding = unpack_coding_e::ENCODE_BY_BIG_ENDIAN;
	setting->body_offset = MessagePacket::PackLenth;
	setting->length_field_bytes = 1;
	setting->length_field_offset = 0;
	pSSock->setUnpack(setting);
	pSSock->setThreadNum(4);

	pServerEntityMan = new ServerEntityManager;
	pServerEntityMan->Init();

	return true;
}

void ControlServer::InitCmd(map<string, function<void(stringstream *)>> &cmdMap)
{
	
}

bool ControlServer::Start()
{
	if(!pSSock)
	{
		DNPrint(ErrCode_SrvNotInit, LoggerLevel::Error, nullptr);
		return false;
	}
	
	pSSock->Start();
	return true;
}

bool ControlServer::Stop()
{
	if(pSSock)
	{
		pSSock->End();
	}
	return true;
}

void ControlServer::Pause()
{
	LoopEvent([](EventLoopPtr loop)
	{ 
		loop->pause(); 
	});
}

void ControlServer::Resume()
{
	LoopEvent([](EventLoopPtr loop)
	{ 
		loop->resume(); 
	});
}

void ControlServer::LoopEvent(function<void(EventLoopPtr)> func)
{
    map<long,EventLoopPtr> looped;
    if(pSSock)
	{
		while(EventLoopPtr pLoop = pSSock->loop())
		{
			long id = pLoop->tid();
			if(looped.find(id) == looped.end())
			{
				func(pLoop);
				looped[id] = pLoop;
			}
			else
			{
				break;
			}
		};
	}
}
