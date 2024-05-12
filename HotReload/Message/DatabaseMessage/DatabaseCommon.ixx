module;
#include <coroutine>
#include <cstdint>
#include "hv/Channel.h"

#include "StdAfx.h"
#include "Server/S_Common.pb.h"
export module DatabaseMessage:DatabaseCommon;

import DNTask;
import MessagePack;
import DatabaseServerHelper;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg;

namespace DatabaseMessage
{

	// client request
	export DNTaskVoid Evt_ReqRegistSrv()
	{
		DatabaseServerHelper* dnServer = GetDatabaseServer();
		DNClientProxyHelper* client = dnServer->GetCSock();
		uint32_t msgId = client->GetMsgId();
		
		// first Can send Msg?
		if(client->GetMsg(msgId))
		{
			DNPrint(0, LoggerLevel::Debug, "+++++ %lu, ", msgId);
			co_return;
		}
		else
		{
			DNPrint(0, LoggerLevel::Debug, "Client:%s, port:%hu", client->remote_host.c_str(), client->remote_port);
		}

		client->RegistState() = RegistState::Registing;

		COM_ReqRegistSrv requset;
		requset.set_server_type((int)dnServer->GetServerType());
		requset.set_server_index(dnServer->ServerIndex());
		
		// pack data
		string binData;
		binData.resize(requset.ByteSizeLong());
		requset.SerializeToArray(binData.data(), binData.size());
		MessagePack(msgId, MsgDeal::Req, requset.GetDescriptor()->full_name().c_str(), binData);
		
		// data alloc
		COM_ResRegistSrv response;
		auto taskGen = [&response]() -> DNTask<Message>
		{
			co_return response;
		};

		auto dataChannel = taskGen();


		{
			// wait data parse
			client->AddMsg(msgId, &dataChannel);
			client->send(binData);
			co_await dataChannel;
			if(dataChannel.HasFlag(DNTaskFlag::Timeout))
			{
				DNPrint(0, LoggerLevel::Debug, "requst timeout! ");
			}
		}
		
		if(response.success())
		{
			DNPrint(0, LoggerLevel::Debug, "regist Server success! ");
			client->RegistState() = RegistState::Registed;
			dnServer->ServerIndex() = response.server_index();
		}
		else
		{
			DNPrint(0, LoggerLevel::Debug, "regist Server error! msg:%lu ", msgId);
			dnServer->IsRun() = false; //exit application
			client->RegistState() = RegistState::None;
		}

		dataChannel.Destroy();
		co_return;
	}

	export void Exe_RetChangeCtlSrv(const SocketChannelPtr &channel, uint32_t msgId, Message *msg)
	{
		COM_RetChangeCtlSrv* requset = reinterpret_cast<COM_RetChangeCtlSrv*>(msg);
		DatabaseServerHelper* dnServer = GetDatabaseServer();

		dnServer->ReClientEvent(requset->ip(), requset->port());
	}
}
