module;
export module DNWebProxy;

import ThirdParty.Libhv;
import ECSW;
import Logger;

export class DNWebProxy : public Component, public hv::HttpServer
{
protected:
	friend class System;
	DNWebProxy(System::Ptr system):Component(system)
	{
		eComponentType = EMComponentType::DNWebProxy;
	}
public:
	~DNWebProxy(){}

	bool Awake() override
	{
		World::Ptr world = GetOwner()->GetWorld();

		uint16_t port = 0;
		std::string* value = world->LaunchParam("port");
		if (value)
		{
			port = stoi(*value);
		}

		setHost("0.0.0.0");
		setPort(port);
		setThreadNum(4);

		LoggerPrint::Ptr pLogger = GetOwner()->GetWorld()->GetSystem<LoggerPrint>(EMSystemType::LoggerPrint);
		pLogger->Record(EL10nCode_SrvListenOn, port, 0);
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
