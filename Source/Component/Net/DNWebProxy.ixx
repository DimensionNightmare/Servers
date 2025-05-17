module;
export module DNWebProxy;

import ThirdParty.Libhv;
import ECSW;
import Logger;

export class DNWebProxy : public Component, public hv::HttpServer
{
protected:
	friend class System;
	DNWebProxy(System::WPtr system):Component(system)
	{
		eComponentType = EMComponentType::DNWebProxy;
	}
public:
	using Ptr = std::shared_ptr<DNWebProxy>;
	using WPtr = std::weak_ptr<DNWebProxy>;
	~DNWebProxy() = default;

	virtual void Dispose() override
	{
		Component::Dispose();
	}

	bool Awake() override
	{
		World::Ptr pWorld = GetOwner()->GetWorld();

		uint16_t port = 0;
		std::string* value = pWorld->LaunchParam("port");
		if (value)
		{
			port = stoi(*value);
		}

		setHost("0.0.0.0");
		setPort(port);
		setThreadNum(4);

		LoggerPrint::Ptr pLogger = pWorld->GetSystem<LoggerPrint>(EMSystemType::LoggerPrint);
		pLogger->Record(EL10nCode_SrvListenOn, port, 0);

		GetOwner()->AddEvent(EMEventType::ServerStart, GetSelfW<DNWebProxy>(), &DNWebProxy::Start);

		hv::HttpService* service = new hv::HttpService();

		service->POST("/Auth/User/LoginToken", [](const hv::HttpRequestPtr& req, const hv::HttpResponseWriterPtr& writer)
		{
			writer->Begin();
			int a = 0;
			int c = 3 / a;
			writer->End();
		});

		registerHttpService(service);

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
};
