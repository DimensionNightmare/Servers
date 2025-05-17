module;
export module LogicServerHelper;

export import DNServer;
import DNClientProxyHelper;
import DNServerProxyHelper;
import RoomEntityManagerHelper;
import ClientEntityManagerHelper;
import Logger;
import ThirdParty.RedisPP;
import MdbProxy;

export class LogicServerHelper : public DNServer
{

private:

	LogicServerHelper() = delete;
	~LogicServerHelper() = default;

	LogicServerHelper(const LogicServerHelper&) = delete;
	// void operator=(const LogicServerHelper&) = delete;

	LogicServerHelper(LogicServerHelper&&) = delete;
	LogicServerHelper& operator=(LogicServerHelper&&) = delete;

	void* operator new(size_t) = delete;
    void operator delete(void*) = delete;
public:
	using Ptr = std::shared_ptr<LogicServerHelper>;

	DNClientProxyHelper::Ptr GetClientProxy() { return GetComponent<DNClientProxyHelper>(EMComponentType::DNClientProxy); }

	DNServerProxyHelper::Ptr GetServerProxy() { return GetComponent<DNServerProxyHelper>(EMComponentType::DNServerProxy); }

	RoomEntityManagerHelper::Ptr GetRoomEntityManager() { return GetComponent<RoomEntityManagerHelper>(EMComponentType::RoomEntityManager); }

	ClientEntityManagerHelper::Ptr GetClientEntityManager() { return GetComponent<ClientEntityManagerHelper>(EMComponentType::ClientEntityManager); }

	MdbProxy::Ptr GetMdbProxy(){ return GetComponent<MdbProxy>(EMComponentType::MdbProxy); }

	bool InitDatabase()
	{
		if(MdbProxy::Ptr proxy = GetMdbProxy())
		{
			try
			{
				World::Ptr pWorld = GetWorld();

				std::string* value = pWorld->LaunchParam("connection");

				auto connection = std::make_shared<sw::redis::Redis>(*value);
				connection->ping();

				proxy->AddConnection(std::move(connection));
			}
			catch (const std::exception& e)
			{
				GetLogger()->Record(ELogLevel_Debug, e.what());
				return false;
			}

			return true;
		}

		return false;
	}

};
