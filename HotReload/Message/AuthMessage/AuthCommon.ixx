module;
#include <coroutine>
#include <cstdint>
#include <string>
#include <memory>

#include "StdMacro.h"
export module AuthMessage:AuthCommon;

import DNTask;
import FuncHelper;
import AuthServerHelper;
import Logger;
import ThirdParty.PbGen;

namespace AuthMessage
{

	// client request
	export DNTaskVoid Evt_ReqRegistSrv()
	{
		AuthServerHelper* dnServer = GetAuthServer();
		DNClientProxyHelper* client = dnServer->GetCSock();
		DNWebProxyHelper* server = dnServer->GetSSock();
		uint32_t msgId = client->GetMsgId();

		DNPrint(0, LoggerLevel::Debug, "Client:%s, port:%hu", client->remote_host.c_str(), client->remote_port);
		
		client->RegistState() = RegistState::Registing;

		COM_ReqRegistSrv request;
		request.set_server_type((int)dnServer->GetServerType());

		if (uint32_t serverIndex = dnServer->ServerId())
		{
			request.set_server_id(serverIndex);
		}

		request.set_server_port(server->port);

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
}