module;
export module DNServer;

import ThirdParty.Libhv;
import ECSW;

export class DNServer : public System
{
	friend class World;
	DNServer(World::Ptr world):System(world)
	{
		eSystemType = EMSystemType::DNServer;

		Libhv::hvlog_disable();
	}
public:
	using Ptr = std::shared_ptr<DNServer>;

	virtual ~DNServer()
	{
	}

	virtual bool Awake() override
	{
		if (std::string* value = GetWorld()->LaunchParam("svrIndex"))
		{
			iServerId = stoi(*value);
		}

		return true;
	}

	void Start()	{ 	eServerEvent.Broadcast(EMEventType::ServerStart	); }

	void Stop() 	{ 	eServerEvent.Broadcast(EMEventType::ServerStop	); }

	void Pause()	{ 	eServerEvent.Broadcast(EMEventType::ServerPause	); }

	void Resume()	{ 	eServerEvent.Broadcast(EMEventType::ServerResume); }

	EMServerType GetServerType() { return emServerType; }

	void SetServerType(EMServerType type) { emServerType = type; }

	uint32_t& ServerId() { return iServerId; }

	Event& GetEvent(){return eServerEvent;}

public: // dll override

protected:

	EMServerType emServerType = EMServerType::None;

	uint32_t iServerId = 0;

	std::mutex oTaskMutex;

	Event eServerEvent;
};
