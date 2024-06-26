module;
#include <coroutine>
#include <cstdint>
#include <string>
#include <memory>

#include "StdMacro.h"
export module DatabaseMessage:DatabaseCommon;

import DNTask;
import FuncHelper;
import DatabaseServerHelper;
import Logger;
import Macro;
import ThirdParty.Libhv;
import ThirdParty.PbGen;

namespace DatabaseMessage
{

	// client request
	export DNTaskVoid Evt_ReqRegistSrv()
	{
		DatabaseServerHelper* dnServer = GetDatabaseServer();
		DNClientProxyHelper* client = dnServer->GetCSock();
		
		DNPrint(0, LoggerLevel::Debug, "Client:%s, port:%hu", client->remote_host.c_str(), client->remote_port);
		
		client->RegistState() = RegistState::Registing;

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
			MessagePackAndSend(msgId, MsgDeal::Req, request.GetDescriptor()->full_name().c_str(), binData, client->GetChannel());

			co_await dataChannel;
			if (dataChannel.HasFlag(DNTaskFlag::Timeout))
			{
				DNPrint(0, LoggerLevel::Debug, "requst timeout! ");
			}

		}

		if (response.success())
		{
			DNPrint(0, LoggerLevel::Debug, "regist Server success! Rec index:%d", response.server_id());
			client->RegistState() = RegistState::Registed;
			client->RegistType() = response.server_type();
			dnServer->ServerId() = response.server_id();
		}
		else
		{
			DNPrint(0, LoggerLevel::Debug, "regist Server error!  ");
			// dnServer->IsRun() = false; //exit application
			client->RegistState() = RegistState::None;
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
