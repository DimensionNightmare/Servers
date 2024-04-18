module;
#include <Thread>
#include <iostream>
#include "hv/EventLoop.h"
#include "hv/hsocket.h"

#include "StdAfx.h"
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

	virtual void InitCmd(map<string, function<void(stringstream *)>> &cmdMap) override;

	virtual bool Start() override;

	virtual bool Stop() override;

	virtual void Pause() override;

	virtual void Resume() override;

	virtual void LoopEvent(function<void(EventLoopPtr)> func) override;

public: // dll override
	virtual DNClientProxy *GetCSock() { return pCSock; }

protected: // dll proxy
	DNClientProxy *pCSock;

	// record orgin info
	string sCtlIp;
	unsigned short iCtlPort;
};

DatabaseServer::DatabaseServer()
{
	emServerType = ServerType::DatabaseServer;
	pCSock = nullptr;
}

DatabaseServer::~DatabaseServer()
{
	if (pCSock)
	{
		pCSock->setReconnect(nullptr);
		delete pCSock;
		pCSock = nullptr;
	}
}

bool DatabaseServer::Init()
{
	string *value = GetLuanchConfigParam("byCtl");
	if (!value || !stoi(*value))
	{
		DNPrint(1, LoggerLevel::Error, nullptr);
		return false;
	}

	DNServer::Init();

	unsigned short port = 0;

	unpack_setting_t *setting = new unpack_setting_t;
	setting->mode = unpack_mode_e::UNPACK_BY_LENGTH_FIELD;
	setting->length_field_coding = unpack_coding_e::ENCODE_BY_BIG_ENDIAN;
	setting->body_offset = MessagePacket::PackLenth;
	setting->length_field_bytes = 1;
	setting->length_field_offset = 0;

	// connet ControlServer
	string *ctlPort = GetLuanchConfigParam("ctlPort");
	string *ctlIp = GetLuanchConfigParam("ctlIp");
	if (ctlPort && ctlIp && is_ipaddr(ctlIp->c_str()))
	{
		pCSock = new DNClientProxy;
		pCSock->pLoop = make_shared<EventLoopThread>();

		reconn_setting_t *reconn = new reconn_setting_t;
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
	if (pCSock) // client
	{
		pCSock->pLoop->start();
		pCSock->start();
	}

	return true;
}

bool DatabaseServer::Stop()
{
	if (pCSock) // client
	{
		pCSock->pLoop->stop(true);
		pCSock->stop();
	}

	return true;
}

void DatabaseServer::Pause()
{
	LoopEvent([](hv::EventLoopPtr loop)
			  { loop->pause(); });
}

void DatabaseServer::Resume()
{
	LoopEvent([](hv::EventLoopPtr loop)
			  { loop->resume(); });
}

void DatabaseServer::LoopEvent(function<void(EventLoopPtr)> func)
{
	map<long, EventLoopPtr> looped;

	if (pCSock)
	{
		looped.clear();
		while (EventLoopPtr pLoop = pCSock->loop())
		{
			long id = pLoop->tid();
			if (looped.find(id) == looped.end())
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
