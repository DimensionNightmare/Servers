module;
export module LogicMessage:LogicGate;

import LogicServerHelper;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import ClientEntityManagerHelper;
import std.compat;

namespace LogicMessage
{
	export void Exe_RetProxyOffline(SocketChannelPtr channel, std::string binMsg)
	{
		g2L_RetProxyOffline request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}

		LogicServerHelper* dnServer = GetLogicServer();
		ClientEntityManagerHelper* entityMan = dnServer->GetClientEntityManager();

		if (ClientEntity* entity = entityMan->GetEntity(request.entity_id()))
		{
			LoggerPrint()(ELogLevel_Debug, "Recv Client {} Disconnect !!", entity->ID());

			entityMan->SaveEntity(*entity, true);
			entityMan->RemoveEntity(entity->ID());
			return;
		}

		LoggerPrint()(ELogLevel_Debug, "Recv Client {} Disconnect but not Exist!!", request.entity_id());
	}
}