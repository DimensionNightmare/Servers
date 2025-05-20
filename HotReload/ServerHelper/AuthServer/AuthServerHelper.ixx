module;
export module AuthServerHelper;

import DNServer;
import DNClientProxyHelper;
import DNWebProxyHelper;
import DbUtils;
import Logger;
import ThirdParty.Libpqxx;
import RdbProxy;
import StrUtils;

export class AuthServerHelper : public DNServer
{

private:

	AuthServerHelper() = delete;
	~AuthServerHelper() = default;

	AuthServerHelper(const AuthServerHelper&) = delete;
	// void operator=(const AuthServerHelper&) = delete;

	AuthServerHelper(AuthServerHelper&&) = delete;
	AuthServerHelper& operator=(AuthServerHelper&&) = delete;

	void* operator new(size_t) = delete;
    void operator delete(void*) = delete;
public:
	using Ptr = std::shared_ptr<AuthServerHelper>;

	DNClientProxyHelper::Ptr GetClientProxy() { return GetComponent<DNClientProxyHelper>(EMComponentType::DNClientProxy); }

	DNWebProxyHelper::Ptr GetWebProxy() { return GetComponent<DNWebProxyHelper>(EMComponentType::DNWebProxy); }

	RdbProxy::Ptr GetRdbProxy(){ return GetComponent<RdbProxy>(EMComponentType::RdbProxy); }

	bool InitDatabase()
	{
		
		if(RdbProxy::Ptr proxy = GetRdbProxy())
		{
			try
			{
				World::Ptr pWorld = GetWorld();

				//"postgresql://root@localhost"
				std::string* value = pWorld->LaunchParam("connection");

				std::string* dbName = pWorld->LaunchParam("dbname");

				auto connection = std::make_shared<pqxx::connection>(std::format("{} dbname = {}", *value, *dbName));

				proxy->AddConnection((uint16_t)EnumName<EMSqlDbNameEnum>(*dbName), std::move(connection));
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
