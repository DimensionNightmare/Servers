module;
export module MdbProxy;

import ECSW;


export class MdbProxy : public Component
{
protected:
	friend class System;
	MdbProxy(System::WPtr system):Component(system)
	{
		eComponentType = EMComponentType::MdbProxy;
	}

public:
	~MdbProxy() = default;

	virtual void Dispose() override
	{
		Component::Dispose();
	}

protected:
};
