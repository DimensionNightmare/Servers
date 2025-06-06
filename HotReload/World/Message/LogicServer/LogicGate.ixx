module;
export module LogicServerMessage:LogicGate;

import LogicServerHelper;

import ThirdParty.Libhv;

namespace LogicServerMessage
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

			entityMan->SaveEntity(entity->GetSelf<ClientEntityHelper>(), true);
			entityMan->RemoveEntity(entity->ID());
			return;
		}

		dnServer->GetLogger()->Record(ELogLevel_Debug, "Recv Client {} Disconnect but not Exist!!", request.entity_id());
	}
}