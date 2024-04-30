module;
#include <coroutine>
#include <random>
#include <format>
#include <cstdint>
#include <list>
#include "hv/Channel.h"

#include "StdAfx.h"
#include "Server/S_Control_Global.pb.h"
#include "Server/S_Global_Gate.pb.h"
#include "Server/S_Common.pb.h"
export module GlobalMessage:GlobalControl;

import DNTask;
import MessagePack;
import GlobalServerHelper;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg;

#define CastObj(entity) static_cast<ServerEntityHelper*>(entity)

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


	string binData;
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
		binData.clear();
		binData.resize(msg->ByteSizeLong());
		msg->SerializeToArray(binData.data(), binData.size());
		MessagePack(smsgId, MsgDeal::Req, G2g_ReqLoginToken::GetDescriptor()->full_name().c_str(), binData);
		
		// data alloc
		auto dataChannel = [&tokenRes]()->DNTask<Message>
		{
			co_return tokenRes;
		}();

		{
			server->AddMsg(smsgId, &dataChannel, 8000);
			entity->GetSock()->write(binData);
			co_await dataChannel;
			if(dataChannel.HasFlag(DNTaskFlag::Timeout))
			{
				DNPrint(0, LoggerLevel::Debug, "requst timeout! \n");
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

	binData.clear();
	binData.resize(response.ByteSizeLong());
	response.SerializeToArray(binData.data(), binData.size());

	MessagePack(msgId, MsgDeal::Res, nullptr, binData);
	
	channel->write(binData);

	co_return;
}