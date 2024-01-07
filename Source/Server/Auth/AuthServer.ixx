module;
#include "hv/hasync.h"
#include "hv/EventLoop.h"
#include "hv/hsocket.h"

export module AuthServer;

import DNServer;
import DNWebProxy;
import DNClientProxy;
import MessagePack;

using namespace std;
using namespace hv;

export class AuthServer : public DNServer
{
public:
	AuthServer();

	~AuthServer();

	virtual bool Init(map<string, string> &param) override;

	virtual void InitCmd(map<string, function<void(stringstream*)>> &cmdMap) override;

	virtual bool Start() override;

	virtual bool Stop() override;

	virtual void Pause() override;

	virtual void Resume() override;

	virtual void LoopEvent(function<void(EventLoopPtr)> func) override;

public: // dll override
	virtual DNWebProxy* GetSSock(){return pSSock;}
	virtual DNClientProxy* GetCSock(){return pCSock;}
protected: // dll proxy
	DNWebProxy* pSSock;
	DNClientProxy* pCSock;
};

module:private;

AuthServer::AuthServer()
{
	emServerType = ServerType::AuthServer;
	pSSock = nullptr;
	pCSock = nullptr;
}

AuthServer::~AuthServer()
{
	Stop();

	if(pSSock)
	{
		delete pSSock;
		pSSock = nullptr;
	}

	if(pCSock)
	{
		pCSock->setReconnect(nullptr);
		delete pCSock;
		pCSock = nullptr;
	}
}

bool AuthServer::Init(map<string, string> &param)
{
	int port = 0;
	
	if(param.contains("port"))
	{
		port = stoi(param["port"]);
	}

	pSSock = new DNWebProxy;
	pSSock->setPort(port);
	pSSock->setThreadNum(4);

	//connet ControlServer
	if(stoi(param["byCtl"]) && param.contains("ctlPort") && param.contains("ctlIp"))
	{
		auto setting = new unpack_setting_t;
		setting->mode = unpack_mode_e::UNPACK_BY_LENGTH_FIELD;
		setting->length_field_coding = unpack_coding_e::ENCODE_BY_BIG_ENDIAN;
		setting->body_offset = MessagePacket::PackLenth;
		setting->length_field_bytes = 1;
		setting->length_field_offset = 0;
		
		pCSock = new DNClientProxy;
		auto reconn = new reconn_setting_t;
		reconn->min_delay = 1000;
		reconn->max_delay = 10000;
		reconn->delay_policy = 2;
		pCSock->setReconnect(reconn);
		port = stoi(param["ctlPort"]);
		pCSock->createsocket(port, param["ctlIp"].c_str());
		pCSock->setUnpack(setting);
	}

	return true;
}

void AuthServer::InitCmd(map<string, function<void(stringstream *)>> &cmdMap)
{
	
}

bool AuthServer::Start()
{
	if(!pSSock)
	{
		fprintf(stderr, "%s->Server not Initialed! \n", __FUNCTION__);
		return false;
	}
	pSSock->start();

	if(pCSock)
	{
		pCSock->start();
	}
	
	return true;
}

bool AuthServer::Stop()
{
	pSSock->stop();
	
	if(pCSock)
	{
		pCSock->stop();
	}
	async::cleanup();
	return true;
}

void AuthServer::Pause()
{
	pSSock->stop();

	LoopEvent([](hv::EventLoopPtr loop)
	{ 
		loop->pause(); 
	});
}

void AuthServer::Resume()
{
	pSSock->start();

	LoopEvent([](hv::EventLoopPtr loop)
	{ 
		loop->resume(); 
	});
}

void AuthServer::LoopEvent(function<void(EventLoopPtr)> func)
{
    map<long,EventLoopPtr> looped;
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