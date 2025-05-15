module;
export module DNSocketProxy;

import ThirdParty.Libhv;
import ECSW;

export class DNSocketProxy : public hv::SocketChannel
{
public:
	using Ptr = std::shared_ptr<DNSocketProxy>;
	using WPtr = std::weak_ptr<DNSocketProxy>;

	virtual ~DNSocketProxy() {}

	DNSocketProxy(hv::hio_t* io) : SocketChannel(io)
	{
	}


	void SetWorld(World::WPtr world) { pWorld = world; }

	World::Ptr GetWorld() { return pWorld.expired() ? nullptr : pWorld.lock(); }

protected:

	World::WPtr pWorld;
};
