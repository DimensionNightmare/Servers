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
		for(auto world : oWorlds)
		{
			Server::Ptr dnServer = world->GetSystem<Server>(EMSystemType::Server);
	
			switch (dnServer->GetServerType())
			{
				#define one(Type) case EMServerType::Type:							\
				{ 																	\
					AddEvent(EMEventType::InitHotReload, 							\
						dnServer->GetSelfW<Type##Helper>(),							\
						&Type##Helper::HandleServerInit); 							\
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
		for(auto world : oWorlds)
		{
			Server::Ptr dnServer = world->GetSystem<Server>(EMSystemType::Server);
	
			switch (dnServer->GetServerType())
			{
				#define one(Type) case EMServerType::Type:							\
				{ 																	\
					AddEvent(EMEventType::DeinitHotReload,							\
						dnServer->GetSelfW<Type##Helper>(),							\
						&Type##Helper::HandleServerShutdown); 						\
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