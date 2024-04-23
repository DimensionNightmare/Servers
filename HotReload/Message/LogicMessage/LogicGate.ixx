module;
#include <coroutine>
#include "hv/Channel.h"

#include "StdAfx.h"
#include "Server/S_Gate_Logic.pb.h"
#include "Server/S_Logic_Dedicated.pb.h"
export module LogicMessage:LogicGate;

import DNTask;
import MessagePack;
import LogicServerHelper;

using namespace std;
using namespace google::protobuf;
using namespace hv;
using namespace GMsg;

// client request
export DNTaskVoid Msg_ReqClientLogin(const SocketChannelPtr &channel, uint32_t msgId, Message *msg)
{
	G2L_ReqClientLogin* requset = reinterpret_cast<G2L_ReqClientLogin*>(msg);

	LogicServerHelper* dnServer = GetLogicServer();
	auto entityMan = dnServer->GetClientEntityManager();

	ClientEntityHelper* entity = nullptr;
	if (entity = entityMan->AddEntity(requset->account_id()))
	{
		DNPrint(0, LoggerLevel::Debug, "AddEntity Client!");

	}
	else
	{
		DNPrint(0, LoggerLevel::Debug, "AddEntity Exist Client!");
		entity =  entityMan->GetEntity(requset->account_id());
	}


	auto serverEntityMan = dnServer->GetServerEntityManager();
	ServerEntityHelper* serverEntity = nullptr;
	
	// cache
	if(uint32_t serverIdx = entity->ServerIndex())
	{
		serverEntity = serverEntityMan->GetEntity(serverIdx);
	}
	else
	{
		list<ServerEntity*> serverEntityList = serverEntityMan->GetEntityByList(ServerType::DedicatedServer);
		if(!serverEntityList.empty())
		{
			serverEntity = static_cast<ServerEntityHelper*>(serverEntityList.front());
		}
	}

	L2G_ResClientLogin response;

	string binData;

	// req token
	if(serverEntity)
	{
		response.set_ip(serverEntity->ServerIp());
		response.set_port(serverEntity->ServerPort());
		
		auto dataChannel = [&response]()->DNTask<Message>
		{
			co_return response;
		}();

		DNServerProxyHelper* server = dnServer->GetSSock();
		uint32_t smsgId = server->GetMsgId();

		binData.resize(msg->ByteSize());
		msg->SerializeToArray(binData.data(), binData.size());
		MessagePack(smsgId, MsgDeal::Req, L2D_ReqClientLogin::GetDescriptor()->full_name().c_str(), binData);
		
		{

			// wait data parse
			server->AddMsg(smsgId, &dataChannel, 9000);
			serverEntity->GetSock()->write(binData);
			co_await dataChannel;
			if(dataChannel.HasFlag(DNTaskFlag::Timeout))
			{
				DNPrint(0, LoggerLevel::Debug, "requst timeout! \n");
			}

			binData.clear();
		}

		DNPrint(0, LoggerLevel::Debug, "ds:%s", serverEntity->ServerIp().c_str());
	}
	else
	{
		response.set_state_code(1);
		DNPrint(0, LoggerLevel::Debug, "not ds connect");
	}

	// pack data
	binData.resize(response.ByteSizeLong());
	response.SerializeToArray(binData.data(), binData.size());
	MessagePack(msgId, MsgDeal::Res, nullptr, binData);

	channel->write(binData);

	co_return;
}
