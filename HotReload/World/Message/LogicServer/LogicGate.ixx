export module LogicServerMessage:LogicGate;

import std;
import LogicServerHelper;
import ThirdParty.Libhv;
import ThirdParty.PbGen;

export namespace LogicServerMessage
{
	void Exe_RetProxyOffline(SocketChannel::CVPtr channel, const std::string& binMsg)
	{
		GMsg::g2L_RetProxyOffline request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}

		LogicServerHelper::CVPtr dnServer = channel->GetWorld()->GetSystem<LogicServerHelper>(EMSystemType::Server);
		ClientEntityManagerHelper::CVPtr entityMan = dnServer->GetClientEntityManager();

		if (ClientEntity::CVPtr entity = entityMan->GetEntity(request.entity_id()))
		{
			dnServer->GetLogger()->Record(ELogLevel_Debug, "Recv Client {} Disconnect !!", entity->ID());

			entityMan->SaveEntity(entity->GetSelf<ClientEntityHelper>(), true);
			entityMan->RemoveEntity(entity->ID());
			return;
		}

		dnServer->GetLogger()->Record(ELogLevel_Debug, "Recv Client {} Disconnect but not Exist!!", request.entity_id());
	}
}