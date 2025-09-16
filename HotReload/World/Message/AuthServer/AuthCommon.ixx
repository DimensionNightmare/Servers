export module AuthServerMessage:AuthCommon;

import AuthServerHelper;
import Server;
import FuncHelper;
import Task;
import ThirdParty.PbGen;


export namespace AuthServerMessage
{

	// client request
	TaskVoid Evt_ReqRegistSrv(Server::CVPtr server)
	{
		AuthServerHelper::CVPtr dnServer = server->GetSelf<AuthServerHelper>();

		ClientProxyHelper::CVPtr clientProxy = dnServer->GetClientProxy();

		uint32_t msgId = clientProxy->GetMsgId();

		dnServer->GetLogger()->Record(ELogLevel_Debug, "Client:{}, port:{}", clientProxy->remote_host, clientProxy->remote_port);
		
		clientProxy->SetRegistState(EMRegistState::Registing);

		GMsg::COM_ReqRegistSrv request;
		request.set_server_id(dnServer->ID());
		request.set_server_type((int)dnServer->GetServerType());

		if (dnServer->IsPullServer())
		{
			request.set_is_pull(true);
		}

		if(WebProxyHelper::CVPtr serverProxy = dnServer->GetWebProxy())
		{
			request.set_server_port(serverProxy->port);
		}


		// pack data
		std::string binData;
		request.SerializeToString(&binData);
		
		// data alloc
		GMsg::COM_ResRegistSrv response;

		{
			auto taskGen = [](Message* msg) -> Task<Message*>
				{
					co_return msg;
				};
			auto dataChannel = taskGen(&response);
			
			uint32_t msgId = clientProxy->GetMsgId();
			clientProxy->AddMsg(msgId, &dataChannel);
			MessagePackAndSend(msgId, EMMsgDeal::Req, request.GetDescriptor()->full_name(), binData, clientProxy->GetChannel());

			co_await dataChannel;
			if (dataChannel.HasFlag(EMTaskFlag::Timeout))
			{
				response.set_error_code(EL10nCode_ReqRegistTimeout);
			}

		}

		if (response.error_code() == EL10nCode_None)
		{
			clientProxy->SetRegistState(EMRegistState::Registed);
			clientProxy->SetRegistType(response.ret_server_type());
		}
		else
		{
			dnServer->GetLogger()->Record(response.error_code());
			// server->IsRun() = false; //exit application
			clientProxy->SetRegistState(EMRegistState::None);
		}

		co_return;
	}
}