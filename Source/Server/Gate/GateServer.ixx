module;
#include "StdMacro.h"
export module GateServer;

import DNServer;
import DNServerProxy;
import DNClientProxy;
import ServerEntityManager;
import ProxyEntityManager;
import Logger;
import Config.Server;
import ThirdParty.Libhv;
import ThirdParty.PbGen;

export class GateServer : public DNServer
{

public:

	GateServer()
	{
		emServerType = EMServerType::GateServer;
	}

	// need init order reversal
	~GateServer()
	{
		pSSock = nullptr;
		pCSock = nullptr;
		pServerEntityMan = nullptr;
		pProxyEntityMan = nullptr;
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

		pSSock = std::make_unique<DNServerProxy>();

		int listenfd = pSSock->createsocket(port, "0.0.0.0");
		if (listenfd < 0)
		{
			DNPrintCode(EL10nCode_CreateSocket);
			return false;
		}

		pSSock->Init();

		DNPrintCode(EL10nCode_SrvListenOn, pSSock->port, listenfd);

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

		pServerEntityMan = std::make_unique<ServerEntityManager>();
		pServerEntityMan->Init();
		pProxyEntityMan = std::make_unique<ProxyEntityManager>();
		pProxyEntityMan->Init();

		return true;
	}

	virtual void InitCmd( std::unordered_map<std::string, std::function<void(std::stringstream*)>>& cmdMap) override
	{
	}

	virtual bool Start() override
	{

		if (pCSock) // client
		{
			pCSock->Start();
		}

		if (!pSSock)
		{
			DNPrintCode(EL10nCode_SrvNotInit);
			return false;
		}

		pSSock->Start();
		return true;
	}

	virtual bool Stop() override
	{
		if (pSSock)
		{
			pSSock->End();
		}

		if (pCSock) // client
		{
			pCSock->End();
		}

		return true;
	}


	virtual void Pause() override
	{
		// pSSock->Timer()->pause();
		// pCSock->Timer()->pause();
		// pServerEntityMan->Timer()->pause();

		LoopEvent([](EventLoopPtr loop)
		{
			loop->pause();
		});
	}

	virtual void Resume() override
	{
		LoopEvent([](EventLoopPtr loop)
		{
			loop->resume();
		});

		// pSSock->Timer()->resume();
		// pCSock->Timer()->resume();
		// pServerEntityMan->Timer()->resume();
	}

	virtual void LoopEvent(std::function<void(EventLoopPtr)> func) override
	{
		std::unordered_map<long, bool> looped;
		if (pSSock)
		{
			looped.clear();
			while (const EventLoopPtr& pLoop = pSSock->loop())
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

	virtual DNServerProxy* GetSSock() { return pSSock.get(); }

	virtual DNClientProxy* GetCSock() { return pCSock.get(); }

	virtual ServerEntityManager* GetServerEntityManager() { return pServerEntityMan.get(); }

	virtual ProxyEntityManager* GetProxyEntityManager() { return pProxyEntityMan.get(); }

protected: // dll proxy

	std::unique_ptr<DNServerProxy> pSSock;

	std::unique_ptr<DNClientProxy> pCSock;

	std::unique_ptr<ServerEntityManager> pServerEntityMan;
	
	std::unique_ptr<ProxyEntityManager> pProxyEntityMan;
};
