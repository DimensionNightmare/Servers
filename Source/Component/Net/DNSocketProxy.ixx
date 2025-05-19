module;
export module DNSocketProxy;

import ThirdParty.Libhv;
import ECSW;

export class DNSocketProxy : public hv::SocketChannel
{
public:
	using Ptr = std::shared_ptr<DNSocketProxy>;
	using WPtr = std::weak_ptr<DNSocketProxy>;

	virtual ~DNSocketProxy()
	{
		
	}

	DNSocketProxy(hv::hio_t* io) : SocketChannel(io)
	{
	}


	// void SetWorld(World::Ptr world) 
	// {
	// 	pWorld = world;
	// }

	// World::Ptr GetWorld() { return pWorld; }

protected:

	// World::Ptr pWorld;
};
