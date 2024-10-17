module;
#include "StdMacro.h"
export module NetEntity;

export import Entity;
import ThirdParty.Libhv;

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
	const SocketChannelPtr& GetSock() { return pSock; }

	/// @brief net socket get
	void SetSock(const SocketChannelPtr& channel) { pSock = channel; }

	/// @brief the this close timedown destroy timerid.
	/// @brief authenticate,shutdown and reconnect waiting.
	uint64_t& TimerId() { return iCloseTimerId; }

protected: // dll proxy

	SocketChannelPtr pSock;

	uint64_t iCloseTimerId = 0;
	
};
