module;
export module MdbProxy;

import ECSW;


export class MdbProxy : public Component
{
protected:
	friend class System;
	MdbProxy(System::Ptr system):Component(system)
	{

	}

public:
	~MdbProxy() = default;

protected:
};
