module;
export module RdbProxy;

import ECSW;
import ThirdParty.Libpqxx;

export class RdbProxy : public Component
{
protected:
	friend class System;
	RdbProxy(System::Ptr system):Component(system)
	{
		
	}

	bool Awake()
	{
		return true;
	}

public:
	~RdbProxy() = default;

protected:
	std::unique_ptr<pqxx::connection> pMdbProxy;

};
