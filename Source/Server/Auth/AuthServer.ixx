module;
#include "StdMacro.h"
export module AuthServer;

import DNServer;
import DNWebProxy;
import DNClientProxy;
import Logger;
import Config.Server;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import ThirdParty.Libpqxx;

export class AuthServer : public DNServer
{
	
public:
	AuthServer()
	{
		emServerType = EMServerType::AuthServer;
	}

	// need init order reversal
	~AuthServer()
	{
		// proxy
		pCSock = nullptr;
		pSSock = nullptr;
		// other:db
		pSqlProxy = nullptr;
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

		value = LaunchConfig::GetParam("port");
		if (value)
		{
			port = stoi(*value);
		}

		pSSock = std::make_unique<DNWebProxy>();
		pSSock->setHost("0.0.0.0");
		pSSock->setPort(port);
		pSSock->setThreadNum(4);

		DNPrintCode(EL10nCode_SrvListenOn, pSSock->port, 0);

		//connet ControlServer
		std::string* ctlPort = LaunchConfig::GetParam("ctlPort");
		std::string* ctlIp = LaunchConfig::GetParam("ctlIp");
		if (ctlPort && ctlIp)
		{
			pCSock = std::make_unique<DNClientProxy>();

			pCSock->Init();

			port = stoi(*ctlPort);
			pCSock->createsocket(port, ctlIp->c_str());
		}

		return true;
	}

	virtual void InitCmd( std::unordered_map<std::string, std::function<void(std::stringstream*)>>& cmdMap) override
	{
	}

	/// @brief s->c
	virtual bool Start() override
	{
		if (!pSSock)
		{
			DNPrintCode(EL10nCode_SrvNotInit);
			return false;
		}
		int code = pSSock->Start();
		if (code < 0)
		{
			DNPrint(ELogLevel_Debug, "start error %d", code);
			return false;
		}

		if (pCSock)
		{
			pCSock->Start();
		}

		return true;
	}

	/// @brief c->s
	virtual bool Stop() override
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

	/// @brief c->s
	virtual void Pause() override
	{
		// pCSock->Timer()->pause();

		LoopEvent([](EventLoopPtr loop)
			{
				loop->pause();
			});

		pSSock->stop();
	}

	/// @brief s->c
	virtual void Resume() override
	{
		pSSock->start();

		LoopEvent([](EventLoopPtr loop)
			{
				loop->resume();
			});

		// pCSock->Timer()->resume();
	}

	virtual void LoopEvent(std::function<void(EventLoopPtr)> func) override
	{
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

	pq_connection* SqlProxy() { return pSqlProxy.get(); }

public: // dll override

	virtual DNWebProxy* GetSSock() { return pSSock.get(); }

	virtual DNClientProxy* GetCSock() { return pCSock.get(); }

protected: // dll proxy

	std::unique_ptr<DNWebProxy> pSSock;

	std::unique_ptr<DNClientProxy> pCSock;

	std::unique_ptr<pq_connection> pSqlProxy;
	
};
