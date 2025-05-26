module;
export module MdbProxy;

import ThirdParty.RedisPP;
import ECSW;
import std.compat;

export class MdbProxy : public Component
{
protected:
	friend class System;
	MdbProxy(System::WPtr system):Component(system)
	{
		eComponentType = EMComponentType::MdbProxy;
	}

public:
	using Ptr = std::shared_ptr<MdbProxy>;
	
	~MdbProxy() = default;

	virtual void Dispose() override
	{
		Component::Dispose();
	}

	void AddConnection(std::shared_ptr<sw::redis::Redis>&& connection)
	{
		pMdbProxys.emplace(0, std::move(connection));
	}

	std::shared_ptr<sw::redis::Redis> GetConnection()
	{
		if (pMdbProxys.contains(0))
		{
			return pMdbProxys[0];
		}
		return nullptr;
	}

	void ClearConnections()
	{
		pMdbProxys.clear();
	}

protected:
	std::unordered_map<uint16_t, std::shared_ptr<sw::redis::Redis>> pMdbProxys;
};
