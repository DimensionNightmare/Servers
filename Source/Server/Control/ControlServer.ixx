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
		string* port = GetLuanchConfigParam("port");
		if (!port)
		{
			DNPrint(ErrCode::ErrCode_SrvNeedIPPort, EMLoggerLevel::Error, nullptr);
			return false;
		}

		DNServer::Init();

		pSSock = make_unique<DNServerProxy>();

		int listenfd = pSSock->createsocket(stoi(*port), "0.0.0.0");
		if (listenfd < 0)
		{
			DNPrint(ErrCode::ErrCode_CreateSocket, EMLoggerLevel::Error, nullptr);
			return false;
		}

		pSSock->Init();

		DNPrint(TipCode::TipCode_SrvListenOn, EMLoggerLevel::Normal, nullptr, pSSock->port, listenfd);

		pServerEntityMan = make_unique<ServerEntityManager>();
		pServerEntityMan->Init();

		return true;
	}

	virtual void InitCmd(unordered_map<string, function<void(stringstream*)>>& cmdMap) override
	{
	}

	virtual bool Start() override
	{
		if (!pSSock)
		{
			DNPrint(ErrCode::ErrCode_SrvNotInit, EMLoggerLevel::Error, nullptr);
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

	virtual void LoopEvent(function<void(EventLoopPtr)> func) override
	{
		unordered_map<long, bool> looped;
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

	unique_ptr<DNServerProxy> pSSock;

	unique_ptr<ServerEntityManager> pServerEntityMan;
};
