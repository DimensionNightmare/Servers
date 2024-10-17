module;
#include "StdMacro.h"
export module LogicServer;

export import DNServer;
import DNServerProxy;
import DNClientProxy;
import ServerEntityManager;
import ClientEntityManager;
import RoomEntityManager;
import Logger;
import Config.Server;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import ThirdParty.RedisPP;

export class LogicServer : public DNServer
{

public:

	LogicServer()
	{
		emServerType = ServerType::LogicServer;
	}

	// need init order reversal
	~LogicServer()
	{
		pSSock = nullptr;
		pCSock = nullptr;
		pRoomMan = nullptr;
		pClientEntityMan = nullptr;
		pNoSqlProxy = nullptr;
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

			sCtlIp = *ctlIp;
			iCtlPort = port;
		}

		pClientEntityMan = make_unique<ClientEntityManager>();
		pClientEntityMan->Init();
		pRoomMan = make_unique<RoomEntityManager>();
		pRoomMan->Init();

		return true;
	}

	virtual void InitCmd(unordered_map<string, function<void(stringstream*)>>& cmdMap) override
	{
		DNServer::InitCmd(cmdMap);

	}

	virtual bool Start() override
	{
		if (!pSSock)
		{
			DNPrint(ErrCode::ErrCode_SrvNotInit, LoggerLevel::Error, nullptr);
			return false;
		}

		pSSock->Start();

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

		if (pSSock)
		{
			pSSock->End();
		}
		return true;
	}

	virtual void Pause() override
	{
		// pSSock->Timer()->pause();
		// pCSock->Timer()->pause();
		// pClientEntityMan->Timer()->pause();
		// pRoomMan->Timer()->pause();

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
		// pClientEntityMan->Timer()->resume();
		// pRoomMan->Timer()->resume();
	}

	virtual void LoopEvent(function<void(EventLoopPtr)> func) override
	{
		unordered_map<long, bool> looped;
		if (pSSock)
		{
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

	virtual void TickMainFrame() override
	{
		pClientEntityMan->TickMainFrame();
	}
public: // dll override

	virtual DNServerProxy* GetSSock() { return pSSock.get(); }

	virtual DNClientProxy* GetCSock() { return pCSock.get(); }

	virtual ClientEntityManager* GetClientEntityManager() { return pClientEntityMan.get(); }

	virtual RoomEntityManager* GetRoomEntityManager() { return pRoomMan.get(); }


protected: // dll proxy

	unique_ptr<DNServerProxy> pSSock;

	unique_ptr<DNClientProxy> pCSock;

	unique_ptr<ClientEntityManager> pClientEntityMan;

	unique_ptr<RoomEntityManager> pRoomMan;
	// record orgin info
	string sCtlIp;
	
	uint16_t iCtlPort = 0;

	// localdb
	shared_ptr<Redis> pNoSqlProxy;
};
