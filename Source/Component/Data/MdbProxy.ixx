export module MdbProxy;

import ThirdParty.RedisPP;
import ECSW;
import std.compat;
import Logger;
import Timer;

export class MdbProxy : public Component
{
protected:
	friend class UniversalMemoryPool;
	MdbProxy(System::CVPtr system):Component(system)
	{
		eComponentType = EMComponentType::MdbProxy;
	}

public:
	using Ptr = std::shared_ptr<MdbProxy>;
	using CVPtr = const Ptr&;
	
	virtual ~MdbProxy()
	{
	}

	virtual void Dispose() override
	{
		pMdbProxys.clear();
		
		Component::Dispose();
	}

	virtual bool Awake() override
	{
		GetWorld()->AddEvent<&MdbProxy::InitDatabase>(EMEventType::ServerStart, GetSelf<MdbProxy>());
		return true;
	}

	void InitDatabase()
	{
		World::CVPtr world = GetWorld();

		std::string* param = world->GetParam("mdbConnection");
		if(!param)
		{
			return;
		}

		auto connection = P_InstanceHolder->GetMemPool().Allocate<sw::redis::Redis>(*param);
		
		try
		{
			connection->ping();
		}
		catch(sw::redis::IoError& e)
		{
			LoggerPrint::Log(world, ELogLevel_Debug, "Can Connect Redis:{}, retest", *param);
			// 重试 retest
			Timer::CVPtr timer = world->GetSystem<Timer>(EMSystemType::Timer);

			timer->SetTimeout(3000, [this](size_t)
			{
				InitDatabase();
			});
			return;
		}
		catch(const std::exception& e)
		{
			LoggerPrint::Log(world, ELogLevel_Debug, "Can Connect Redis:{}, no retest", e.what());
			return;
		}


		pMdbProxys.emplace(0, connection);
	}

protected:

	std::unordered_map<uint16_t, std::shared_ptr<sw::redis::Redis>> pMdbProxys;
};
