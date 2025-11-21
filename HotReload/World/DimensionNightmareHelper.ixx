export module DimensionNightmareHelper;

import FuncUtils;
import DimensionNightmare;
import GlobalServerHelper;
import ControlServerHelper;
import AuthServerHelper;
import GateServerHelper;
import DatabaseServerHelper;
import LogicServerHelper;
import Server;

export class DimensionNightmareHelper : public Helper<DimensionNightmareHelper, DimensionNightmare>
{

private:

	DimensionNightmareHelper() = delete;
	~DimensionNightmareHelper() = default;

public:

	void InitHotReload()
	{
		for(const auto& world : oWorlds)
		{
			Server::CVPtr dnServer = world->GetSystem<Server>(EMSystemType::Server);
	
			switch (dnServer->GetServerType())
			{
				#define one(Type) case EMServerType::Type:							\
				{ 																	\
					AddEvent<&Type##Helper::HandleServerInit>(						\
						EMEventType::InitHotReload, 								\
						dnServer->GetSelf<Type##Helper>()); 						\
					break;															\
				}
				one(ControlServer)
				one(GlobalServer)
				one(AuthServer)
				one(GateServer)
				one(DatabaseServer)
				one(LogicServer)
	
				#undef one
			}
		}
	}

	void DeInitHotReload()
	{
		for(const auto& world : oWorlds)
		{
			Server::CVPtr dnServer = world->GetSystem<Server>(EMSystemType::Server);
	
			switch (dnServer->GetServerType())
			{
				#define one(Type) case EMServerType::Type:							\
				{ 																	\
					AddEvent<&Type##Helper::HandleServerShutdown>(					\
						EMEventType::DeinitHotReload, 								\
						dnServer->GetSelf<Type##Helper>()); 						\
					break;															\
				}
				one(ControlServer)
				one(GlobalServer)
				one(AuthServer)
				one(GateServer)
				one(DatabaseServer)
				one(LogicServer)
	
				#undef one
			}
		}
	}
};