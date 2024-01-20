module;
#include "hv/EventLoop.h"
#include "hv/hsocket.h"

export module GateServer;

import DNServer;
import DNServerProxy;
import DNClientProxy;
import MessagePack;
import AfxCommon;
import ServerEntity;
import ServerEntityManager;

#define DNPrint(fmt, ...) printf("[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr().c_str(), __FUNCTION__, ##__VA_ARGS__);
#define DNPrintErr(fmt, ...) fprintf(stderr, "[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr().c_str(), __FUNCTION__, ##__VA_ARGS__);

using namespace std;
using namespace hv;

export class GateServer : public DNServer
{
public:
	GateServer();

	~GateServer();

	virtual bool Init(map<string, string> &param) override;

	virtual void InitCmd(map<string, function<void(stringstream*)>> &cmdMap) override;

	virtual bool Start() override;

	virtual bool Stop() override;

	virtual void Pause() override;

	virtual void Resume() override;

	virtual void LoopEvent(function<void(EventLoopPtr)> func) override;

public: // dll override
	virtual DNServerProxy* GetSSock(){return pSSock;}
	virtual DNClientProxy* GetCSock(){return pCSock;}

	virtual ServerEntityManager<ServerEntity>* GetEntityManager(){return pEntityMan;}

protected: // dll proxy
	DNServerProxy* pSSock;
	DNClientProxy* pCSock;

	ServerEntityManager<ServerEntity>* pEntityMan;
};

module:private;

GateServer::GateServer()
{
	emServerType = ServerType::GateServer;
	pSSock = nullptr;
	pCSock = nullptr;

	pEntityMan = nullptr;
}

GateServer::~GateServer()
{
	Stop();

	delete pEntityMan;
	pEntityMan = nullptr;

	if(pSSock)
	{
		pSSock->setUnpack(nullptr);
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

bool GateServer::Init(map<string, string> &param)
{
	if(!param.count("byCtl"))
	{
		DNPrintErr("Server need by Control! \n");
		return false;
	}

	int port = 0;
	
	if(param.contains("port"))
	{
		port = stoi(param["port"]);
	}
	
	pSSock = new DNServerProxy;

	int listenfd = pSSock->createsocket(port);
	if (listenfd < 0)
	{
		DNPrintErr("createsocket error! \n");
		return false;
	}

	// if not set port mean need get port by self 
	if(port == 0)
	{
		struct sockaddr_in addr;
		socklen_t addrLen = sizeof(addr);
		if (getsockname(listenfd, (struct sockaddr*)&addr, &addrLen) < 0) 
		{
			DNPrintErr("Error in getsockname \n");
			return false;
		}

		pSSock->port = ntohs(addr.sin_port);
	}
	
	DNPrint("pSSock listen on port %d, listenfd=%d ... \n", pSSock->port, listenfd);

	auto setting = new unpack_setting_t;
	setting->mode = unpack_mode_e::UNPACK_BY_LENGTH_FIELD;
	setting->length_field_coding = unpack_coding_e::ENCODE_BY_BIG_ENDIAN;
	setting->body_offset = MessagePacket::PackLenth;
	setting->length_field_bytes = 1;
	setting->length_field_offset = 0;
	pSSock->setUnpack(setting);
	pSSock->setThreadNum(4);

	
	//connet ControlServer
	if(param.contains("ctlPort") && param.contains("ctlIp") && is_ipaddr(param["ctlIp"].c_str()))
	{
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
	
	pEntityMan = new ServerEntityManager<ServerEntity>;

	return true;
}

void GateServer::InitCmd(map<string, function<void(stringstream *)>> &cmdMap)
{
}

bool GateServer::Start()
{
	if(!pSSock)
	{
		DNPrintErr("Server not Initialed! \n");
		return false;
	}

	pSSock->start();

	if(pCSock) // client
	{
		pCSock->start();
	}

	return true;
}

bool GateServer::Stop()
{
	if(pSSock)
	{
		pSSock->stop();
	}

	if(pCSock) // client
	{
		pCSock->stop();
	}

	return true;
}

void GateServer::Pause()
{
	LoopEvent([](hv::EventLoopPtr loop)
	{ 
		loop->pause(); 
	});
}

void GateServer::Resume()
{
	LoopEvent([](hv::EventLoopPtr loop)
	{ 
		loop->pause(); 
	});
}

void GateServer::LoopEvent(function<void(EventLoopPtr)> func)
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