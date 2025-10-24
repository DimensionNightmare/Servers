export module WebProxy;

import Logger;
import ECSW;
import std.compat;
import ThirdParty.Libhv;

export class WebProxy : public Component, public hv::HttpServer
{
protected:

	friend class UniversalMemoryPool;
	WebProxy(System::WPtr system):Component(system)
	{
		eComponentType = EMComponentType::WebProxy;
	}
public:
	using Ptr = std::shared_ptr<WebProxy>;
	using CVPtr = const Ptr&;
	using WPtr = std::weak_ptr<WebProxy>;
	virtual ~WebProxy()
	{

	}

	virtual void Dispose() override
	{
		End();
		
		Component::Dispose();
	}

	bool Awake() override
	{
		World::CVPtr world = GetWorld();

		uint16_t port = 0;
		std::string* value = world->LaunchParam("port");
		if (value)
		{
			port = stoi(*value);
		}

		setHost("0.0.0.0");
		setPort(port);
		setThreadNum(1);

		LoggerPrint::Log(GetWorld(), EL10nCode_SrvListenOn, port, 0);

		GetWorld()->AddEvent(EMEventType::ServerStart, GetSelfW<WebProxy>(), &WebProxy::Start);

		pService = P_InstanceHolder->MemPool->Allocate<hv::HttpService>();
		
		service = pService.get();
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

protected:

	std::shared_ptr<hv::HttpService> pService;

};
