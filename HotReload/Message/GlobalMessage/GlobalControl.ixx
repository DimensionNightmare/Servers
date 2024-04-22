module;
#include <coroutine>
#include <random>
#include <format>
#include "hv/Channel.h"

#include "StdAfx.h"
#include "Server/S_Control_Global.pb.h"
#include "Server/S_Global_Gate.pb.h"
#include "Server/S_Common.pb.h"
export module GlobalMessage:GlobalControl;

import DNTask;
import MessagePack;
import GlobalServerHelper;
import ServerEntityHelper;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg;

#define CastObj(entity) static_cast<ServerEntityHelper*>(entity)

// client request
export DNTaskVoid Evt_ReqRegistSrv()
{
	GlobalServerHelper* dnServer = GetGlobalServer();
	DNClientProxyHelper* client = dnServer->GetCSock();
	DNServerProxyHelper* server = dnServer->GetSSock();
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
	if(server->host == "0.0.0.0")
	{
		requset.set_ip("127.0.0.1");
	}
	else
	{
		requset.set_ip(server->host);
	}
	requset.set_port(server->port);
	
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

export DNTaskVoid Msg_ReqAuthAccount(const SocketChannelPtr &channel, uint32_t msgId, Message *msg)
{
	G2C_ResAuthAccount response;

	// if has db not need origin
	list<ServerEntity*> servList = GetGlobalServer()->GetServerEntityManager()->GetEntityByList(ServerType::GateServer);

	list<ServerEntityHelper*> tempList;
	for(ServerEntity* it : servList)
	{
		ServerEntityHelper* gate = CastObj(it);
		if(gate->HasFlag(ServerEntityFlag::Locked))
		{
			tempList.emplace_back(gate);
		}
	}
		
	tempList.sort([](ServerEntityHelper* lhs, ServerEntityHelper* rhs){ return lhs->GetConnNum() < rhs->GetConnNum(); });


	if(!tempList.empty())
	{
		ServerEntityHelper* entity = tempList.front();
		entity->GetConnNum()++;
		response.set_ip( entity->ServerIp());
		response.set_port( entity->ServerPort());

		g2G_ResLoginToken tokenRes;

		DNServerProxyHelper* server = GetGlobalServer()->GetSSock();
		uint32_t smsgId = server->GetMsgId();
		
		// pack data
		string binData;
		binData.resize(msg->ByteSizeLong());
		msg->SerializeToArray(binData.data(), (int)binData.size());
		MessagePack(smsgId, MsgDeal::Req, G2g_ReqLoginToken::GetDescriptor()->full_name().c_str(), binData);
		
		// data alloc
		auto dataChannel = [&tokenRes]()->DNTask<Message>
		{
			co_return tokenRes;
		}();

		{
			server->AddMsg(smsgId, &dataChannel);
			entity->GetSock()->write(binData);
			co_await dataChannel;
			if(dataChannel.HasFlag(DNTaskFlag::Timeout))
			{

			}
		}

		response.set_token(tokenRes.token());
		response.set_expired_timespan(tokenRes.expired_timespan());

		dataChannel.Destroy();
	}
	else
	{
		response.set_state_code(2);
	}

	string binData;
	binData.resize(response.ByteSize());
	response.SerializeToArray(binData.data(), binData.size());

	MessagePack(msgId, MsgDeal::Res, nullptr, binData);
	
	channel->write(binData);

	co_return;
}