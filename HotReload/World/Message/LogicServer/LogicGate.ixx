export module LogicServerMessage:LogicGate;

import std;
import LogicServerHelper;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import Logger;
import MessagePack;
import LogicServerMessage;

namespace MsgHandleRegister
{

	HandleRegistry<GMsg::g2L_RetProxyOffline, void, EMMsgDeal::Ret> Exe_RetProxyOffline(
				[](auto request, SocketChannel::Ptr channel)
	{
		LogicServerHelper::Ptr dnServer = channel->GetWorld()->GetSystem<LogicServerHelper>(EMSystemType::Server);
		ClientEntityManagerHelper::Ptr entityMan = dnServer->GetClientEntityManager();

		if (ClientEntity::Ptr entity = entityMan->GetEntity(request->entityid()))
		{
			LoggerPrint::Log(channel, ELogLevel_Debug, "Recv Client {} Disconnect !!", entity->ID());

			entityMan->SaveEntity(entity->GetSelf<ClientEntityHelper>(), true);
			entityMan->RemoveEntity(entity->ID());
			return;
		}

		LoggerPrint::Log(channel, ELogLevel_Debug, "Recv Client {} Disconnect but not Exist!!", request->entityid());
	});
}