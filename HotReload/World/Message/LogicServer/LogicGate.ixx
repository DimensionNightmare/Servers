export module LogicServerMessage:LogicGate;

import LogicServerHelper;
import ThirdParty.PbGen;
import Logger;
import LogicServerMessage;

namespace MsgHandleRegister
{

	HandleRegistry<GMsg::g2L_RetProxyOffline, void, EMMsgDeal::Ret> Exe_RetProxyOffline =
				[](World::Ptr world, SocketChannel::Ptr channel, auto request)
	{
				LogicServerHelper::CVPtr dnServer = world->GetSystem<LogicServerHelper>(EMSystemType::Server);
		ClientEntityManagerHelper::CVPtr entityMan = dnServer->GetClientEntityManager();

		if (ClientEntity::CVPtr entity = entityMan->GetEntity(request->entityid()))
		{
			LoggerPrint::Log(world, ELogLevel_Debug, "Recv Client {} Disconnect !!", entity->ID());

			entityMan->SaveEntity(entity->GetSelf<ClientEntityHelper>(), true);
			entityMan->DisposeEntity(entity);
			return;
		}

		LoggerPrint::Log(world, ELogLevel_Debug, "Recv Client {} Disconnect but not Exist!!", request->entityid());
	};
}