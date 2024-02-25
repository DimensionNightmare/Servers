module;
#include "StdAfx.h"
#include "hv/EventLoop.h"
#include "hv/hsocket.h"

export module DatabaseServer;

import DNServer;
import DNServerProxy;
import DNClientProxy;
import MessagePack;

using namespace std;
using namespace hv;

export class DatabaseServer : public DNServer
{
public:
	DatabaseServer();

	~DatabaseServer();

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

protected: // dll proxy
	DNClientProxy* pCSock;

	// record orgin info
	string sCtlIp;
	unsigned short iCtlPort;
};

module:private;

DatabaseServer::DatabaseServer()
{
	emServerType = ServerType::DatabaseServer;
	pCSock = nullptr;
}

DatabaseServer::~DatabaseServer()
{
	Stop();

	if(pCSock)
	{
		pCSock->setReconnect(nullptr);
		delete pCSock;
		pCSock = nullptr;
	}
}

bool DatabaseServer::Init()
{
	string* value = GetLuanchConfigParam("byCtl");
	if(!value || !stoi(*value))
	{
		DNPrint(1, LoggerLevel::Error, nullptr);
		return false;
	}

	DNServer::Init();

	int port = 0;

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
	
	return true;
}

void DatabaseServer::InitCmd(map<string, function<void(stringstream *)>> &cmdMap)
{
}

bool DatabaseServer::Start()
{
	if(pCSock) // client
	{
		pCSock->start();
	}

	return true;
}

bool DatabaseServer::Stop()
{
	if(pCSock) // client
	{
		pCSock->stop();
	}

	return true;
}

void DatabaseServer::Pause()
{
	LoopEvent([](hv::EventLoopPtr loop)
	{ 
		loop->pause(); 
	});
}

void DatabaseServer::Resume()
{
	LoopEvent([](hv::EventLoopPtr loop)
	{ 
		loop->resume(); 
	});
}

void DatabaseServer::LoopEvent(function<void(EventLoopPtr)> func)
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

void DatabaseServer::ReClientEvent(const char* ip, int port)
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
