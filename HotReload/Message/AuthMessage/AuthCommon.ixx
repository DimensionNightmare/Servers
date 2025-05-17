module;
export module AuthMessage:AuthCommon;

import DNTask;
import FuncHelper;
import Logger;
import ThirdParty.PbGen;
import DNClientProxyHelper;
import DNServer;
import AuthServerHelper;

namespace AuthMessage
{

	// client request
	export DNTaskVoid Evt_ReqRegistSrv(const DNServer::Ptr& server)
	{
		AuthServerHelper::Ptr dnServer = server->GetSelf<AuthServerHelper>();

		DNClientProxyHelper::Ptr clientProxy = dnServer->GetClientProxy();

		uint32_t msgId = clientProxy->GetMsgId();

		dnServer->GetLogger()->Record(ELogLevel_Debug, "Client:{}, port:{}", clientProxy->remote_host, clientProxy->remote_port);
		
		clientProxy->EMRegistState() = EMRegistState::Registing;

		GMsg::COM_ReqRegistSrv request;
		request.set_server_type((int)server->GetServerType());

		if (uint32_t serverIndex = server->ServerId())
		{
			request.set_server_id(serverIndex);
		}

		if(DNWebProxyHelper::Ptr serverProxy = server->GetComponent<DNWebProxyHelper>(EMComponentType::DNWebProxy))
		{
			request.set_server_port(serverProxy->port);
		}


		// pack data
		std::string binData;
		request.SerializeToString(&binData);
		
		// data alloc
		GMsg::COM_ResRegistSrv response;

		{
			auto taskGen = [](Message* msg) -> DNTask<Message*>
				{
					co_return msg;
				};
			auto dataChannel = taskGen(&response);
			
			uint32_t msgId = clientProxy->GetMsgId();
			clientProxy->AddMsg(msgId, &dataChannel);
			MessagePackAndSend(msgId, EMMsgDeal::Req, request.GetDescriptor()->full_name(), binData, clientProxy->GetChannel());

			co_await dataChannel;
			if (dataChannel.HasFlag(EMDNTaskFlag::Timeout))
			{
				dnServer->GetLogger()->Record(ELogLevel_Debug, "requst timeout! ");
			}

		}

		if (response.success())
		{
			dnServer->GetLogger()->Record(ELogLevel_Debug, "regist Server success! Rec index:{}", response.server_id());
			clientProxy->EMRegistState() = EMRegistState::Registed;
			clientProxy->RegistType() = response.server_type();
			dnServer->ServerId() = response.server_id();
		}
		else
		{
			dnServer->GetLogger()->Record(ELogLevel_Debug, "regist Server error!  ");
			// server->IsRun() = false; //exit application
			clientProxy->EMRegistState() = EMRegistState::None;
		}

		co_return;
	}
}