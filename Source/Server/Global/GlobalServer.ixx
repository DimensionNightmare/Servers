module;
export module GlobalServer;

import DNServer;
import DNServerProxy;
import DNClientProxy;
import ServerEntityManager;
import Logger;
import Config.Server;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import std.compat;

export class GlobalServer : public DNServer
{

public:

	GlobalServer()
	{
		emServerType = EMServerType::GlobalServer;
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

		std::string* value = LaunchConfig::GetParam("port");
		if (value)
		{
			port = stoi(*value);
		}

		pSSock = std::make_unique<DNServerProxy>();

		int listenfd = pSSock->createsocket(port, "0.0.0.0");
		if (listenfd < 0)
		{
			LoggerPrint()(EL10nCode_CreateSocket);
			return false;
		}

		pSSock->Init();

		LoggerPrint()(EL10nCode_SrvListenOn, pSSock->port, listenfd);

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
			LoggerPrint()(EL10nCode_SrvNotInit);
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

		LoopEvent([](hv::EventLoopPtr loop)
		{
			loop->pause();
		});
	}

	virtual void Resume() override
	{
		LoopEvent([](hv::EventLoopPtr loop)
		{
			loop->resume();
		});

		// pSSock->Timer()->resume();
		// pCSock->Timer()->resume();
		// pServerEntityMan->Timer()->resume();
	}

	virtual void LoopEvent(std::function<void(hv::EventLoopPtr)> func) override
	{
		std::unordered_map<long, bool> looped;
		if (pSSock)
		{
			looped.clear();
			while (const hv::EventLoopPtr& pLoop = pSSock->loop())
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
			while (const hv::EventLoopPtr& pLoop = pCSock->loop())
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

	std::unique_ptr<DNServerProxy> pSSock;
	
	std::unique_ptr<DNClientProxy> pCSock;

	std::unique_ptr<ServerEntityManager> pServerEntityMan;
};
