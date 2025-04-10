module;
#include "StdMacro.h"
export module DatabaseServer;

import DNServer;
import DNClientProxy;
import Logger;
import Config.Server;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import ThirdParty.Libpqxx;

export class DatabaseServer : public DNServer
{

public:
	
	DatabaseServer()
	{
		emServerType = EMServerType::DatabaseServer;
	}


	// need init order reversal
	~DatabaseServer()
	{
		pCSock = nullptr;

		pSqlProxys.clear();
	}

	virtual bool Init() override
	{
		std::string* value = LaunchConfig::GetParam("byCtl");
		if (!value || !stoi(*value))
		{
			DNPrintCode(EL10nCode_SrvByCtl);
			return false;
		}

		DNServer::Init();

		uint16_t port = 0;

		// connet ControlServer
		std::string* ctlPort = LaunchConfig::GetParam("ctlPort");
		std::string* ctlIp = LaunchConfig::GetParam("ctlIp");
		if (ctlPort && ctlIp)
		{
			pCSock = std::make_unique<DNClientProxy>();

			pCSock->Init();

			port = stoi(*ctlPort);
			pCSock->createsocket(port, ctlIp->c_str());

			sCtlIp = *ctlIp;
			iCtlPort = port;
		}

		return true;
	}

	virtual void InitCmd( std::unordered_map<std::string, std::function<void(std::stringstream*)>>& cmdMap) override
	{
		DNServer::InitCmd(cmdMap);
	}

	virtual bool Start() override
	{
		if (pCSock) // client
		{
			pCSock->Start();
		}

		return true;
	}

	virtual bool Stop() override
	{
		if (pCSock) // client
		{
			pCSock->End();
		}

		return true;
	}

	virtual void Pause() override
	{
		// pCSock->Timer()->pause();

		LoopEvent([](EventLoopPtr loop)
		{
			loop->pause();
		});
	}

	virtual void Resume() override {
		LoopEvent([](EventLoopPtr loop)
		{
			loop->resume();
		});

		// pCSock->Timer()->resume();
	}


	virtual void LoopEvent(std::function<void(EventLoopPtr)> func) override {
		std::unordered_map<long, bool> looped;

		if (pCSock)
		{
			looped.clear();
			while (const EventLoopPtr& pLoop = pCSock->loop())
			{
				long id = pLoop->tid();
				if (!looped.contains(id))
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

public: // dll override

	virtual DNClientProxy* GetCSock() { return pCSock.get(); }
protected: // dll proxy

	std::unique_ptr<DNClientProxy> pCSock;

	std::unordered_map<uint16_t, std::unique_ptr<pq_connection>> pSqlProxys;

	// record orgin info
	std::string sCtlIp;

	uint16_t iCtlPort = 0;
};
