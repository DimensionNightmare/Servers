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
	MdbProxy(System::WPtr system):Component(system)
	{
		eComponentType = EMComponentType::MdbProxy;
	}

public:
	using Ptr = std::shared_ptr<MdbProxy>;
	
	virtual ~MdbProxy()
	{
	}

	virtual void Dispose() override
	{
		Component::Dispose();
		
		pMdbProxys.clear();
	}

	virtual bool Awake() override
	{
		GetWorld()->AddEvent<&MdbProxy::InitDatabase>(EMEventType::ServerStart, GetSelfW<MdbProxy>());
		return true;
	}

	void InitDatabase()
	{
		World::Ptr world = GetWorld();

		std::string* param = world->GetParam("connection");
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
			LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "Can Connect Redis:{}, retest", *param);
			// 重试 retest
			Timer::Ptr timer = GetWorld()->GetSystem<Timer>(EMSystemType::Timer);

			timer->SetTimeout(3000, [this](size_t)
			{
				InitDatabase();
			});
			return;
		}
		catch(std::exception& e)
		{
			LoggerPrint::Log(GetWorld(), ELogLevel_Debug, "Can Connect Redis:{}, no retest", e.what());
			return;
		}


		pMdbProxys.emplace(0, std::move(connection));
	}

protected:

	std::unordered_map<uint16_t, std::shared_ptr<sw::redis::Redis>> pMdbProxys;
};
