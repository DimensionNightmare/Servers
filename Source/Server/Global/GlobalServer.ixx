module;
#include "StdMacro.h"
export module GlobalServer;

export import DNServer;
import DNServerProxy;
import DNClientProxy;
import ServerEntityManager;
import Logger;
import Config.Server;
import ThirdParty.Libhv;
import ThirdParty.PbGen;

export class GlobalServer : public DNServer
{

public:

	GlobalServer()
	{
		emServerType = ServerType::GlobalServer;
	}

	// need init order reversal
	~GlobalServer()
	{
		pSSock = nullptr;
		pCSock = nullptr;
		pServerEntityMan = nullptr;
	}

	virtual bool Init() override
	{
		DNServer::Init();

		uint16_t port = 0;

		string* value = GetLuanchConfigParam("port");
		if (value)
		{
			port = stoi(*value);
		}

		pSSock = make_unique<DNServerProxy>();

		int listenfd = pSSock->createsocket(port, "0.0.0.0");
		if (listenfd < 0)
		{
			DNPrint(ErrCode::ErrCode_CreateSocket, LoggerLevel::Error, nullptr);
			return false;
		}

		pSSock->Init();

		DNPrint(TipCode::TipCode_SrvListenOn, LoggerLevel::Normal, nullptr, pSSock->port, listenfd);

		//connet ControlServer
		string* ctlPort = GetLuanchConfigParam("ctlPort");
		string* ctlIp = GetLuanchConfigParam("ctlIp");
		if (ctlPort && ctlIp)
		{
			pCSock = make_unique<DNClientProxy>();

			pCSock->Init();

			port = stoi(*ctlPort);
			pCSock->createsocket(port, ctlIp->c_str());
		}

		pServerEntityMan = make_unique<ServerEntityManager>();
		pServerEntityMan->Init();

		return true;
	}

	virtual void InitCmd(unordered_map<string, function<void(stringstream*)>>& cmdMap) override
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
			DNPrint(ErrCode::ErrCode_SrvNotInit, LoggerLevel::Error, nullptr);
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
protected: // dll proxy

	unique_ptr<DNServerProxy> pSSock;
	
	unique_ptr<DNClientProxy> pCSock;

	unique_ptr<ServerEntityManager> pServerEntityMan;
};
