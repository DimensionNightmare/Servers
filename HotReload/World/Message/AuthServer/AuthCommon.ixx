export module AuthServerMessage:AuthCommon;

import AuthServerHelper;
import Server;
import FuncHelper;
import Task;
import ThirdParty.PbGen;
import Logger;
import AuthServerMessage;
import ApiManager;

namespace MsgHandleRegister
{

	// client request
	HandleClientRegistry Evt_ReqRegistSrv = [](Server::Ptr server)-> TaskVoid
	{
		AuthServerHelper::CVPtr dnServer = server->GetSelf<AuthServerHelper>();

		ClientProxyHelper::CVPtr clientProxy = dnServer->GetClientProxy();

		WebProxyHelper::CVPtr webProxy = dnServer->GetWebProxy();

		World::CVPtr world = server->GetWorld();

		uint32_t msgId = clientProxy->GetMsgId();

		LoggerPrint::Log(world, ELogLevel_Debug, "Client:{}, port:{}", clientProxy->remote_host, clientProxy->remote_port);
		
		clientProxy->SetRegistState(EMRegistState::Registing);

		GMsg::COM_ReqRegistSrv request;
		request.set_serverid(dnServer->ID());
		request.set_servertype(std::to_underlying(dnServer->GetServerType()));

		if (dnServer->IsPullServer())
		{
			request.set_ispull(true);
		}

		request.set_serverport(webProxy->port);

		if(WebProxyHelper::CVPtr serverProxy = dnServer->GetWebProxy())
		{
			request.set_serverport(serverProxy->port);
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
			// server->IsRun() = false; //exit application
			clientProxy->SetRegistState(EMRegistState::None);
		}

		co_return;
	};

	HandleApiRegistry Evt_ReqRegistApi(&ApiInit);
}