module;
#include "StdMacro.h"
export module LogicMessage:LogicGate;

import LogicServerHelper;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import ClientEntityManagerHelper;

namespace LogicMessage
{
	export void Exe_RetProxyOffline(SocketChannelPtr channel, string binMsg)
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
			DNPrint(0, LoggerLevel::Debug, "Recv Client %u Disconnect !!", entity->ID());

			entityMan->SaveEntity(*entity, true);
			entityMan->RemoveEntity(entity->ID());
			return;
		}

		DNPrint(0, LoggerLevel::Debug, "Recv Client %u Disconnect but not Exist!!", request.entity_id());
	}
}