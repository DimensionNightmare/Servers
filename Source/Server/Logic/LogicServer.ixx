module;
#include "StdAfx.h"
#include "hv/EventLoop.h"
#include "hv/hsocket.h"

export module LogicServer;

import DNServer;
import DNServerProxy;
import DNClientProxy;
import MessagePack;
import ServerEntityManager;



using namespace std;
using namespace hv;

export class LogicServer : public DNServer
{
public:
	LogicServer();

	~LogicServer();

	virtual bool Init() override;

	virtual void InitCmd(map<string, function<void(stringstream*)>> &cmdMap) override;

	virtual bool Start() override;

	virtual bool Stop() override;

	virtual void Pause() override;

	virtual void Resume() override;

	virtual void LoopEvent(function<void(EventLoopPtr)> func) override;

	virtual void ReClientEvent(const char* ip, int port) override;

public: // dll override
	virtual DNClientProxy* GetCSock(){return pCSock;}

	virtual ServerEntityManager<ServerEntity>* GetEntityManager(){return pEntityMan;}

protected: // dll proxy
	DNClientProxy* pCSock;

	ServerEntityManager<ServerEntity>* pEntityMan;

	// record orgin info
	string sCtlIp;
	unsigned short iCtlPort;
};



LogicServer::LogicServer()
{
	emServerType = ServerType::LogicServer;
	pCSock = nullptr;

	pEntityMan = nullptr;
}

LogicServer::~LogicServer()
{
	Stop();

	delete pEntityMan;
	pEntityMan = nullptr;

	if(pCSock)
	{
		pCSock->setReconnect(nullptr);
		delete pCSock;
		pCSock = nullptr;
	}
}

bool LogicServer::Init()
{
	string* value = GetLuanchConfigParam("byCtl");
	if(!value || !stoi(*value))
	{
		DNPrint(1, LoggerLevel::Error, nullptr);
		return false;
	}

	DNServer::Init();

	int port = 0;
	value = GetLuanchConfigParam("port");
	if(value)
	{
		port = stoi(*value);
	}

	unpack_setting_t* setting = new unpack_setting_t;
	setting->mode = unpack_mode_e::UNPACK_BY_LENGTH_FIELD;
	setting->length_field_coding = unpack_coding_e::ENCODE_BY_BIG_ENDIAN;
	setting->body_offset = MessagePacket::PackLenth;
	setting->length_field_bytes = 1;
	setting->length_field_offset = 0;

	//connet ControlServer
	string* ctlPort = GetLuanchConfigParam("ctlPort");
	string* ctlIp = GetLuanchConfigParam("ctlIp");
	if(ctlPort && ctlIp && is_ipaddr(ctlIp->c_str()))
	{
		pCSock = new DNClientProxy;
		reconn_setting_t* reconn = new reconn_setting_t;
		reconn->min_delay = 1000;
		reconn->max_delay = 10000;
		reconn->delay_policy = 2;
		pCSock->setReconnect(reconn);
		port = stoi(*ctlPort);
		pCSock->createsocket(port, ctlIp->c_str());
		pCSock->setUnpack(setting);

		sCtlIp = *ctlIp;
		iCtlPort = port;
	}
	
	pEntityMan = new ServerEntityManager<ServerEntity>;

	return true;
}

void LogicServer::InitCmd(map<string, function<void(stringstream *)>> &cmdMap)
{
}

bool LogicServer::Start()
{
	if(pCSock) // client
	{
		pCSock->start();
	}

	return true;
}

bool LogicServer::Stop()
{
	if(pCSock) // client
	{
		pCSock->stop();
	}

	return true;
}

void LogicServer::Pause()
{
	LoopEvent([](hv::EventLoopPtr loop)
	{ 
		loop->pause(); 
	});
}

void LogicServer::Resume()
{
	LoopEvent([](hv::EventLoopPtr loop)
	{ 
		loop->resume(); 
	});
}

void LogicServer::LoopEvent(function<void(EventLoopPtr)> func)
{
    map<long,EventLoopPtr> looped;

	if(pCSock)
	{
		looped.clear();
		while(EventLoopPtr pLoop = pCSock->loop())
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

void LogicServer::ReClientEvent(const char* ip, int port)
{
	reconn_setting_t* reconn_setting = new reconn_setting_t;
	memcpy(reconn_setting, pCSock->reconn_setting, sizeof reconn_setting);
	unpack_setting_t* unpack_setting = pCSock->unpack_setting;
	auto onConnection = pCSock->onConnection;
	auto onMessage = pCSock->onMessage;

	pCSock->stop();
	// delete pCSock;
	pCSock = new DNClientProxy;

	pCSock->reconn_setting = reconn_setting;
	pCSock->unpack_setting = unpack_setting;
	pCSock->onConnection = onConnection;
	pCSock->onMessage = onMessage;

	pCSock->createsocket(port, ip);
	pCSock->start();
}