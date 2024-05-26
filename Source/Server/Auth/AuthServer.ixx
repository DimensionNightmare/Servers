module;
#include <cstdint>
#include "hv/EventLoop.h"
#include "hv/hsocket.h"
#include "pqxx/connection"
#include "hv/EventLoopThread.h"

#include "StdAfx.h"
export module AuthServer;

export import DNServer;
import DNWebProxy;
import DNClientProxy;

using namespace std;
using namespace hv;

export class AuthServer : public DNServer
{
public:
	AuthServer();

	~AuthServer();

	virtual bool Init() override;

	virtual void InitCmd(map<string, function<void(stringstream*)>>& cmdMap) override;

	virtual bool Start() override;

	virtual bool Stop() override;

	virtual void Pause() override;

	virtual void Resume() override;

	virtual void LoopEvent(function<void(EventLoopPtr)> func) override;

	pqxx::connection* SqlProxy() { return pSqlProxy.get(); }

public: // dll override
	virtual DNWebProxy* GetSSock() { return pSSock.get(); }
	virtual DNClientProxy* GetCSock() { return pCSock.get(); }
protected: // dll proxy
	unique_ptr<DNWebProxy> pSSock;
	unique_ptr<DNClientProxy> pCSock;
	unique_ptr<pqxx::connection> pSqlProxy;
};



AuthServer::AuthServer()
{
	emServerType = ServerType::AuthServer;
}

AuthServer::~AuthServer()
{
	pSSock = nullptr;
	pCSock = nullptr;
	pSqlProxy = nullptr;
}

bool AuthServer::Init()
{
	string* value = GetLuanchConfigParam("byCtl");
	if (!value || !stoi(*value))
	{
		DNPrint(ErrCode_SrvByCtl, LoggerLevel::Error, nullptr);
		return false;
	}

	DNServer::Init();

	uint16_t port = 0;

	value = GetLuanchConfigParam("port");
	if (value)
	{
		port = stoi(*value);
	}

	pSSock = make_unique<DNWebProxy>();
	pSSock->setHost("0.0.0.0");
	pSSock->setPort(port);
	pSSock->setThreadNum(4);

	DNPrint(TipCode_SrvListenOn, LoggerLevel::Normal, nullptr, pSSock->port, 0);

	//connet ControlServer
	string* ctlPort = GetLuanchConfigParam("ctlPort");
	string* ctlIp = GetLuanchConfigParam("ctlIp");
	if (ctlPort && ctlIp && is_ipaddr(ctlIp->c_str()))
	{
		pCSock = make_unique<DNClientProxy>();

		pCSock->Init();

		port = stoi(*ctlPort);
		pCSock->createsocket(port, ctlIp->c_str());
	}

	return true;
}

void AuthServer::InitCmd(map<string, function<void(stringstream*)>>& cmdMap)
{

}

// s->c
bool AuthServer::Start()
{
	if (!pSSock)
	{
		DNPrint(ErrCode_SrvNotInit, LoggerLevel::Error, nullptr);
		return false;
	}
	int code = pSSock->Start();
	if (code < 0)
	{
		DNPrint(0, LoggerLevel::Debug, "start error %d", code);
		return false;
	}

	if (pCSock)
	{
		pCSock->Start();
	}

	return true;
}

// c->s
bool AuthServer::Stop()
{
	if (pCSock)
	{
		pCSock->End();
	}

	//webProxy
	if (pSSock)
	{
		pSSock->End();
	}

	return true;
}

// c->s
void AuthServer::Pause()
{
	pCSock->Timer()->pause();

	LoopEvent([](EventLoopPtr loop)
		{
			loop->pause();
		});

	pSSock->stop();
}

// s->c
void AuthServer::Resume()
{
	pSSock->start();

	LoopEvent([](EventLoopPtr loop)
		{
			loop->resume();
		});

	pCSock->Timer()->resume();
}

void AuthServer::LoopEvent(function<void(EventLoopPtr)> func)
{
	map<long, bool> looped;
	if (pCSock)
	{
		looped.clear();
		while (const EventLoopPtr& pLoop = pCSock->loop())
		{
			long id = pLoop->tid();
			if (!looped.count(id))
			{
				func(pLoop);
				looped[id];
			}
			else
			{
				break;
			}
		};
	}
}