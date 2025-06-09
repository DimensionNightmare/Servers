module;
export module DNWebProxy;

import Logger;
import ECSW;
import std.compat;
import ThirdParty.Libhv;

export class DNWebProxy : public Component, public hv::HttpServer
{
protected:
	friend class System;
	DNWebProxy(System::WPtr system):Component(system)
	{
		eComponentType = EMComponentType::DNWebProxy;

		pLogger = GetOwner()->GetWorld()->GetSystemW<LoggerPrint>(EMSystemType::LoggerPrint);
	}
public:
	using Ptr = std::shared_ptr<DNWebProxy>;
	using CVPtr = const Ptr&;
	using WPtr = std::weak_ptr<DNWebProxy>;
	~DNWebProxy()
	{

	}

	virtual void Dispose() override
	{
		End();
		
		Component::Dispose();
	}

	bool Awake() override
	{
		World::CVPtr pWorld = GetOwner()->GetWorld();

		uint16_t port = 0;
		std::string* value = pWorld->LaunchParam("port");
		if (value)
		{
			port = stoi(*value);
		}

		setHost("0.0.0.0");
		setPort(port);
		setThreadNum(1);

		GetLogger()->Record(EL10nCode_SrvListenOn, port, 0);

		GetOwner()->GetWorld()->AddEvent(EMEventType::ServerStart, GetSelfW<DNWebProxy>(), &DNWebProxy::Start);

		service = new hv::HttpService();
		service->Static("/", "./");

		return true;
	}

	int Start()
	{
		return start();
	}

	void End()
	{
		stop();
	}

	LoggerPrint::Ptr GetLogger(){ return pLogger.expired() ? nullptr : pLogger.lock(); }

protected:

	LoggerPrint::WPtr pLogger;

};
