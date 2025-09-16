export module DatabaseServerMessage:DatabaseCommon;

import FuncHelper;
import DatabaseServerHelper;
import Server;
import Task;

export namespace DatabaseServerMessage
{

	// client request
	TaskVoid Evt_ReqRegistSrv(Server::CVPtr server)
	{
		DatabaseServerHelper::CVPtr dnServer = server->GetSelf<DatabaseServerHelper>();

		ClientProxyHelper::CVPtr clientProxy = dnServer->GetClientProxy();
		
		dnServer->GetLogger()->Record(ELogLevel_Debug, "database req regist Client:{}, port:{}", clientProxy->remote_host, clientProxy->remote_port);
		
		clientProxy->SetRegistState(EMRegistState::Registing);

		GMsg::COM_ReqRegistSrv request;

		request.set_server_id(dnServer->ID());
		request.set_server_type((int)dnServer->GetServerType());

		if (dnServer->IsPullServer())
		{
			request.set_is_pull(true);
		}

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
			// pack data
			std::string binData;
			request.SerializeToString(&binData);
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
			// dnServer->IsRun() = false; //exit application
			clientProxy->SetRegistState(EMRegistState::None);
		}

		co_return;
	}

	void Exe_RetChangeCtlSrv(SocketChannel::CVPtr channel, const std::string& binMsg)
	{
		GMsg::COM_RetChangeCtlSrv request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}

		DatabaseServerHelper::CVPtr dnServer = channel->GetWorld()->GetSystem<DatabaseServerHelper>(EMSystemType::Server);

		ClientProxyHelper::CVPtr clientProxy = dnServer->GetClientProxy();

		clientProxy->RedirectClient(request.server_port(), request.server_ip());
	}
}
