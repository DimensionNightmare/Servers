export module LogicServerMessage:LogicGate;

import LogicServerHelper;
import ThirdParty.PbGen;
import Logger;
import LogicServerMessage;

namespace MsgHandleRegister
{

	HandleRegistry<GMsg::g2L_RetProxyOffline, void, EMMsgDeal::Ret> Exe_RetProxyOffline =
				[](auto request, SocketChannel::CVPtr channel)
	{
		LogicServerHelper::CVPtr dnServer = channel->GetWorld()->GetSystem<LogicServerHelper>(EMSystemType::Server);
		ClientEntityManagerHelper::CVPtr entityMan = dnServer->GetClientEntityManager();

		if (ClientEntity::CVPtr entity = entityMan->GetEntity(request->entityid()))
		{
			LoggerPrint::Log(channel, ELogLevel_Debug, "Recv Client {} Disconnect !!", entity->ID());

			entityMan->SaveEntity(entity->GetSelf<ClientEntityHelper>(), true);
			entityMan->RemoveEntity(entity->ID());
			return;
		}

		LoggerPrint::Log(channel, ELogLevel_Debug, "Recv Client {} Disconnect but not Exist!!", request->entityid());
	};
}