module;
#include <coroutine>
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
using namespace GMsg::S_Common;

// client request
export DNTaskVoid Evt_ReqRegistSrv()
{
	DatabaseServerHelper* dnServer = GetDatabaseServer();
	DNClientProxyHelper* client = dnServer->GetCSock();
	uint32_t msgId = client->GetMsgId();
	
	// first Can send Msg?
	if(client->GetMsg(msgId))
	{
		DNPrint(0, LoggerLevel::Debug, "+++++ %lu, \n", msgId);
		co_return;
	}
	else
	{
		DNPrint(0, LoggerLevel::Debug, "Evt_ReqRegistSrv ----- %lu, \n", msgId);
	}

	client->RegistState() = RegistState::Registing;

	COM_ReqRegistSrv requset;
	requset.set_server_type((int)dnServer->GetServerType());
	requset.set_server_index(dnServer->ServerIndex());
	
	// pack data
	string binData;
	binData.resize(requset.ByteSizeLong());
	requset.SerializeToArray(binData.data(), (int)binData.size());
	MessagePack(msgId, MsgDeal::Req, requset.GetDescriptor()->full_name().c_str(), binData);
	
	// data alloc
	COM_ResRegistSrv response;
	auto dataChannel = [&response]()->DNTask<Message>
	{
		co_return response;
	}();


	{
		// wait data parse
		client->AddMsg(msgId, &dataChannel);
		client->send(binData);
		co_await dataChannel;
		if(dataChannel.HasFlag(DNTaskFlag::Timeout))
		{

		}
	}
	
	if(response.success())
	{
		DNPrint(0, LoggerLevel::Debug, "regist Server success! \n");
		client->RegistState() = RegistState::Registed;
		dnServer->ServerIndex() = response.server_index();
	}
	else
	{
		DNPrint(0, LoggerLevel::Debug, "regist Server error! msg:%lu \n", msgId);
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
	DNClientProxyHelper* client = dnServer->GetCSock();

	client->UpdateClientState(Channel::Status::CLOSED);

	dnServer->ReClientEvent(requset->ip(), requset->port());
}