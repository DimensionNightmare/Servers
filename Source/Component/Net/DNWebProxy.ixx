module;
export module DNWebProxy;

import ThirdParty.Libhv;
import ECSW;

export class DNWebProxy : public Component, public hv::HttpServer
{
protected:
	friend class Entity;
	DNWebProxy(Entity::Ptr entity):Component(entity)
	{
		
	}
public:

	DNWebProxy() = default;

	~DNWebProxy() = default;

	int Start()
	{
		return start();
	}

	void End()
	{
		stop();
	}
};
