module;
export module LogicMessage:LogicGate;

import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import ClientEntityManagerHelper;
import std.compat;
import LogicServerHelper;
import ECSW;

namespace LogicMessage
{
	export void Exe_RetProxyOffline(const DNSocketChannel::Ptr& channel, const std::string& binMsg)
	{
		GMsg::g2L_RetProxyOffline request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}

		LogicServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<LogicServerHelper>(EMSystemType::DNServer);
		ClientEntityManagerHelper::Ptr entityMan = dnServer->GetClientEntityManager();

		if (ClientEntity::Ptr entity = entityMan->GetEntity(request.entity_id()))
		{
			dnServer->GetLogger()->Record(ELogLevel_Debug, "Recv Client {} Disconnect !!", entity->ID());

			entityMan->SaveEntity(entity, true);
			entityMan->RemoveEntity(entity->ID());
			return;
		}

		dnServer->GetLogger()->Record(ELogLevel_Debug, "Recv Client {} Disconnect but not Exist!!", request.entity_id());
	}
}