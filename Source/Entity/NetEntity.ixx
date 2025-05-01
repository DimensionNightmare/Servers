module;
export module NetEntity;

import Entity;
import ThirdParty.Libhv;
import std.compat;

export class NetEntity : public Entity
{
	
protected:
	//dont new this class
	NetEntity(uint32_t id) : Entity(id)
	{
	}

public:
	virtual ~NetEntity()
	{
		pSock = nullptr;
	}

public: // dll override
	/// @brief net socket set
	const hv::SocketChannelPtr& GetSock() { return pSock; }

	/// @brief net socket get
	void SetSock(const hv::SocketChannelPtr& channel) { pSock = channel; }

	/// @brief the this close timedown destroy timerid.
	/// @brief authenticate,shutdown and reconnect waiting.
	uint64_t& TimerId() { return iCloseTimerId; }

protected: // dll proxy

	hv::SocketChannelPtr pSock;

	uint64_t iCloseTimerId = 0;
	
};
