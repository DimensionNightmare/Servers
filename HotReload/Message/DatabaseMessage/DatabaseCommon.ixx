module;
#include "StdMacro.h"
export module DatabaseMessage:DatabaseCommon;

import DNTask;
import FuncHelper;
import DatabaseServerHelper;
import Logger;
import Macro;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import DNClientProxyHelper;

namespace DatabaseMessage
{

	// client request
	export DNTaskVoid Evt_ReqRegistSrv()
	{
		DatabaseServerHelper* dnServer = GetDatabaseServer();
		DNClientProxyHelper* client = dnServer->GetCSock();
		
		DNPrint(0, EMLoggerLevel::Debug, "Client:%s, port:%hu", client->remote_host.c_str(), client->remote_port);
		
		client->EMRegistState() = EMRegistState::Registing;

		COM_ReqRegistSrv request;

		request.set_server_type((int)dnServer->GetServerType());

		if (uint32_t serverIndex = dnServer->ServerId())
		{
			request.set_server_id(serverIndex);
		}

		// pack data
		string binData;
		request.SerializeToString(&binData);
		

		// data alloc
		COM_ResRegistSrv response;

		{
			auto taskGen = [](Message* msg) -> DNTask<Message*>
				{
					co_return msg;
				};
			auto dataChannel = taskGen(&response);

			uint32_t msgId = client->GetMsgId();
			client->AddMsg(msgId, &dataChannel);
			MessagePackAndSend(msgId, EMMsgDeal::Req, request.GetDescriptor()->full_name().c_str(), binData, client->GetChannel());

			co_await dataChannel;
			if (dataChannel.HasFlag(EMDNTaskFlag::Timeout))
			{
				DNPrint(0, EMLoggerLevel::Debug, "requst timeout! ");
			}

		}

		if (response.success())
		{
			DNPrint(0, EMLoggerLevel::Debug, "regist Server success! Rec index:%d", response.server_id());
			client->EMRegistState() = EMRegistState::Registed;
			client->RegistType() = response.server_type();
			dnServer->ServerId() = response.server_id();
		}
		else
		{
			DNPrint(0, EMLoggerLevel::Debug, "regist Server error!  ");
			// dnServer->IsRun() = false; //exit application
			client->EMRegistState() = EMRegistState::None;
		}

		co_return;
	}

	export void Exe_RetChangeCtlSrv(SocketChannelPtr channel, string binMsg)
	{
		COM_RetChangeCtlSrv request;
		if(!request.ParseFromString(binMsg))
		{
			return;
		}
		DatabaseServerHelper* dnServer = GetDatabaseServer();
		DNClientProxyHelper* client = dnServer->GetCSock();

		TICK_MAINSPACE_SIGN_FUNCTION(DNClientProxy, RedirectClient, client, request.server_port(), request.server_ip());
	}
}
