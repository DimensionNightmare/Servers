module;
#include "hv/EventLoop.h"

export module ControlServer;

import DNServer;
import DNServerProxy;
import MessagePack;
import ServerEntity;
import ServerEntityManager;
import AfxCommon;

#define DNPrint(code, level, fmt, ...) LoggerPrint(level, code, __FUNCTION__, fmt, ##__VA_ARGS__);


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
	virtual ServerEntityManager<ServerEntity>* GetEntityManager(){return pEntityMan;}

protected: // dll proxy
	DNServerProxy* pSSock;

	ServerEntityManager<ServerEntity>* pEntityMan;
};

module:private;

ControlServer::ControlServer()
{
	emServerType = ServerType::ControlServer;
	pSSock = nullptr;
	pEntityMan = nullptr;
}

ControlServer::~ControlServer()
{
	Stop();
	
	delete pEntityMan;
	pEntityMan = nullptr;

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
		DNPrint(7, LoggerLevel::Error, nullptr);
		return false;
	}

	DNServer::Init();

	pSSock = new DNServerProxy;

	int listenfd = pSSock->createsocket(stoi(*port));
	if (listenfd < 0)
	{
		DNPrint(8, LoggerLevel::Error, nullptr);
		return false;
	}

	DNPrint(1, LoggerLevel::Debug, nullptr, pSSock->port, listenfd);

	unpack_setting_t* setting = new unpack_setting_t;
	setting->mode = unpack_mode_e::UNPACK_BY_LENGTH_FIELD;
	setting->length_field_coding = unpack_coding_e::ENCODE_BY_BIG_ENDIAN;
	setting->body_offset = MessagePacket::PackLenth;
	setting->length_field_bytes = 1;
	setting->length_field_offset = 0;
	pSSock->setUnpack(setting);
	pSSock->setThreadNum(4);

	pEntityMan = new ServerEntityManager<ServerEntity>;

	return true;
}

void ControlServer::InitCmd(map<string, function<void(stringstream *)>> &cmdMap)
{
	
}

bool ControlServer::Start()
{
	if(!pSSock)
	{
		DNPrint(6, LoggerLevel::Error, nullptr);
		return false;
	}
	
	pSSock->start();
	return true;
}

bool ControlServer::Stop()
{
	if(pSSock)
	{
		pSSock->stop();
	}
	return true;
}

void ControlServer::Pause()
{
	LoopEvent([](hv::EventLoopPtr loop)
	{ 
		loop->pause(); 
	});
}

void ControlServer::Resume()
{
	LoopEvent([](hv::EventLoopPtr loop)
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