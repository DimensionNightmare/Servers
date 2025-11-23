export module DatabaseServerMessage:DatabaseCommon;

import FuncHelper;
import DatabaseServerHelper;
import Server;
import Task;
import ThirdParty.PbGen;
import Logger;
import DatabaseServerMessage;

namespace MsgHandleRegister
{

	// client request
	HandleClientRegistry Evt_ReqRegistSrv = [](Server::Ptr server)-> TaskVoid
	{
		DatabaseServerHelper::CVPtr dnServer = server->GetSelf<DatabaseServerHelper>();

		ClientProxyHelper::CVPtr clientProxy = dnServer->GetClientProxy();

		World::CVPtr world = server->GetWorld();
		
		LoggerPrint::Log(world, ELogLevel_Debug, "database req regist Client:{}, port:{}", clientProxy->remote_host, clientProxy->remote_port);
		
		clientProxy->SetRegistState(EMRegistState::Registing);

		GMsg::COM_ReqRegistSrv request;

		request.set_serverid(dnServer->ID());
		request.set_servertype(std::to_underlying(dnServer->GetServerType()));

		if (dnServer->IsPullServer())
		{
			request.set_ispull(true);
		}

		// data alloc
		GMsg::COM_ResRegistSrv response;

		
		bool success = co_await clientProxy->AddMsg(EMMsgDeal::Req, &request, &response);
		
		if (!success)
		{
			response.set_errorcode(EL10nCode_ReqRegistTimeout);
		}

		if (response.errorcode() == EL10nCode_None)
		{
			clientProxy->SetRegistState(EMRegistState::Registed);
			clientProxy->SetRegistType(response.retservertype());
		}
		else
		{
			LoggerPrint::Log(world, response.errorcode());
			// dnServer->IsRun() = false; //exit application
			clientProxy->SetRegistState(EMRegistState::None);
		}

		co_return;
	};

	HandleRegistry<GMsg::COM_RetChangeCtlSrv, void, EMMsgDeal::Ret> Exe_RetChangeCtlSrv =
		[](World::Ptr world, SocketChannel::Ptr channel, auto request)
	{
		
		DatabaseServerHelper::CVPtr dnServer = world->GetSystem<DatabaseServerHelper>(EMSystemType::Server);

		ClientProxyHelper::CVPtr clientProxy = dnServer->GetClientProxy();

		clientProxy->RedirectClient(request->serverport(), request->serverip());
	};
}
