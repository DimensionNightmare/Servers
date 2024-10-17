module;
#include "StdMacro.h"
export module AuthServer;

export import DNServer;
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
		emServerType = ServerType::AuthServer;
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
		string* value = GetLuanchConfigParam("byCtl");
		if (!value || !stoi(*value))
		{
			DNPrint(ErrCode::ErrCode_SrvByCtl, LoggerLevel::Error, nullptr);
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

		DNPrint(TipCode::TipCode_SrvListenOn, LoggerLevel::Normal, nullptr, pSSock->port, 0);

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

		return true;
	}

	virtual void InitCmd(unordered_map<string, function<void(stringstream*)>>& cmdMap) override
	{
	}

	/// @brief s->c
	virtual bool Start() override
	{
		if (!pSSock)
		{
			DNPrint(ErrCode::ErrCode_SrvNotInit, LoggerLevel::Error, nullptr);
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

	virtual void LoopEvent(function<void(EventLoopPtr)> func) override
	{
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

	pq_connection* SqlProxy() { return pSqlProxy.get(); }

public: // dll override

	virtual DNWebProxy* GetSSock() { return pSSock.get(); }

	virtual DNClientProxy* GetCSock() { return pCSock.get(); }

protected: // dll proxy

	unique_ptr<DNWebProxy> pSSock;

	unique_ptr<DNClientProxy> pCSock;

	unique_ptr<pq_connection> pSqlProxy;
	
};
