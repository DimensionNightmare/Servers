module;
#include "StdMacro.h"
export module ControlServer;

import DNServer;
import DNServerProxy;
import MessagePack;
import ServerEntityManager;
import Logger;
import Config.Server;
import ThirdParty.Libhv;
import ThirdParty.PbGen;

export class ControlServer : public DNServer
{

public:

	ControlServer()
	{
		emServerType = EMServerType::ControlServer;
	}

	// need init order reversal
	~ControlServer()
	{
		pSSock = nullptr;

		pServerEntityMan = nullptr;
	}

	virtual bool Init() override
	{
		std::string* port = LaunchConfig::GetParam("port");
		if (!port)
		{
			DNPrintCode(EL10nCode_SrvNeedIPPort);
			return false;
		}

		DNServer::Init();

		pSSock = std::make_unique<DNServerProxy>();

		int listenfd = pSSock->createsocket(stoi(*port), "0.0.0.0");
		if (listenfd < 0)
		{
			DNPrintCode(EL10nCode_CreateSocket);
			return false;
		}

		pSSock->Init();

		DNPrintCode(EL10nCode_SrvListenOn, pSSock->port, listenfd);

		pServerEntityMan = std::make_unique<ServerEntityManager>();
		pServerEntityMan->Init();

		return true;
	}

	virtual void InitCmd( std::unordered_map<std::string, std::function<void(std::stringstream*)>>& cmdMap) override
	{
	}

	virtual bool Start() override
	{
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
		return true;
	}

	virtual void Pause() override
	{
		// pSSock->Timer()->pause();
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
	}

public: // dll override

	virtual DNServerProxy* GetSSock() { return pSSock.get(); }

	virtual ServerEntityManager* GetServerEntityManager() { return pServerEntityMan.get(); }
protected: // dll proxy

	std::unique_ptr<DNServerProxy> pSSock;

	std::unique_ptr<ServerEntityManager> pServerEntityMan;
};
