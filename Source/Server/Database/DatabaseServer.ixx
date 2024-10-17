module;
#include "StdMacro.h"
export module DatabaseServer;

export import DNServer;
import DNServerProxy;
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
		emServerType = ServerType::DatabaseServer;
	}


	// need init order reversal
	~DatabaseServer()
	{
		pCSock = nullptr;

		pSqlProxys.clear();
	}

	virtual bool Init() override
	{
		string* value = GetLuanchConfigParam("byCtl");
		if (!value || !stoi(*value))
		{
			DNPrint(ErrCode::ErrCode_SrvByCtl, LoggerLevel::Error, nullptr);
			return false;
		}

		DNServer::Init();

		uint16_t port = 0;

		// connet ControlServer
		string* ctlPort = GetLuanchConfigParam("ctlPort");
		string* ctlIp = GetLuanchConfigParam("ctlIp");
		if (ctlPort && ctlIp)
		{
			pCSock = make_unique<DNClientProxy>();

			pCSock->Init();

			port = stoi(*ctlPort);
			pCSock->createsocket(port, ctlIp->c_str());

			sCtlIp = *ctlIp;
			iCtlPort = port;
		}

		return true;
	}

	virtual void InitCmd(unordered_map<string, function<void(stringstream*)>>& cmdMap) override
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


	virtual void LoopEvent(function<void(EventLoopPtr)> func) override {
		unordered_map<long, bool> looped;

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

	unique_ptr<DNClientProxy> pCSock;

	unordered_map<uint16_t, unique_ptr<pq_connection>> pSqlProxys;

	// record orgin info
	string sCtlIp;

	uint16_t iCtlPort = 0;
};
