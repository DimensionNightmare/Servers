module;
#include "AuthGlobal.pb.h"
#include "GlobalControl.pb.h"
#include "hv/Channel.h"

#include <coroutine>
#include <random>
#include <format>
export module GlobalMessage:GlobalAuth;

import DNTask;
import MessagePack;
import GlobalServerHelper;
import ServerEntityHelper;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg::AuthGlobal;

#define CastObj(entity) static_cast<ServerEntityHelper*>(entity)

export DNTaskVoid Exe_AuthAccount(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	A2G_AuthAccount* requset = (A2G_AuthAccount*)msg;
	G2A_AuthAccount response;

	// if has db not need origin
	list<ServerEntity*>& servList = GetGlobalServer()->GetEntityManager()->GetEntityByList(ServerType::GateServer);

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


	if(tempList.size())
	{
		ServerEntityHelper* entity = tempList.front();
		entity->GetConnNum()++;
		response.set_state_code(0);
		response.set_ip_addr( format("{}:{}", entity->GetServerIp(), entity->GetServerPort() ));

	}
	else
	{
		response.set_state_code(1);
	}

	string binData;
	binData.resize(response.ByteSize());
	response.SerializeToArray(binData.data(), binData.size());

	MessagePack(msgId, MsgDeal::Res, nullptr, binData);
	
	channel->write(binData);

	co_return;
}