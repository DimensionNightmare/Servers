module;
export module DatabaseMessage:DatabaseCommon;

import DNTask;
import FuncHelper;
import DatabaseServerHelper;
import Logger;
import DllUtils;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import DNClientProxyHelper;

#define FUNCPLACE(func) #func, func

namespace DatabaseMessage
{

	// client request
	export DNTaskVoid Evt_ReqRegistSrv()
	{
		DatabaseServerHelper* dnServer = GetDatabaseServer();
		DNClientProxyHelper* client = dnServer->GetCSock();
		
		LoggerPrint()(ELogLevel_Debug, "Client:{}, port:{}", client->remote_host, client->remote_port);
		
		client->EMRegistState() = EMRegistState::Registing;

		GMsg::COM_ReqRegistSrv request;

		request.set_server_type((int)dnServer->GetServerType());

		if (uint32_t serverIndex = dnServer->ServerId())
		{
			request.set_server_id(serverIndex);
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

			uint32_t msgId = client->GetMsgId();
			client->AddMsg(msgId, &dataChannel);
			MessagePackAndSend(msgId, EMMsgDeal::Req, request.GetDescriptor()->full_name(), binData, client->GetChannel());

			co_await dataChannel;
			if (dataChannel.HasFlag(EMDNTaskFlag::Timeout))
			{
				LoggerPrint()(ELogLevel_Debug, "requst timeout! ");
			}

		}

		if (response.success())
		{
			LoggerPrint()(ELogLevel_Debug, "regist Server success! Rec index:{}", response.server_id());
			client->EMRegistState() = EMRegistState::Registed;
			client->RegistType() = response.server_type();
			dnServer->ServerId() = response.server_id();
		}
		else
		{
			LoggerPrint()(ELogLevel_Debug, "regist Server error!  ");
			// dnServer->IsRun() = false; //exit application
			client->EMRegistState() = EMRegistState::None;
		}

		co_return;
	}

	export void Exe_RetChangeCtlSrv(hv::SocketChannelPtr channel, std::string binMsg)
	{
		GMsg::COM_RetChangeCtlSrv request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}
		DatabaseServerHelper* dnServer = GetDatabaseServer();
		DNClientProxyHelper* client = dnServer->GetCSock();

		TickMainSpaceDll(client, FUNCPLACE(&DNClientProxy::RedirectClient), request.server_port(), request.server_ip());
	}
}
