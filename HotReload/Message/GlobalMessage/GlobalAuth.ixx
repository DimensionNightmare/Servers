module;
#include <coroutine>
#include <random>
#include <format>
#include "hv/Channel.h"

#include "Server/S_Auth.pb.h"
#include "Server/S_Global.pb.h"
export module GlobalMessage:GlobalAuth;

import DNTask;
import MessagePack;
import GlobalServerHelper;
import ServerEntityHelper;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg::S_Auth;
using namespace GMsg::S_Global;

#define CastObj(entity) static_cast<ServerEntityHelper*>(entity)

export DNTaskVoid Msg_ReqAuthAccount(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	A2G_ReqAuthAccount* requset = reinterpret_cast<A2G_ReqAuthAccount*>(msg);
	G2A_ResAuthAccount response;

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
		response.set_state_code(0);
		response.set_ip( entity->ServerIp());
		response.set_port( entity->ServerPort());

		G2G_ReqLoginToken tokenReq;
		tokenReq.set_account_id(requset->account_id());
		tokenReq.set_ip(requset->ip());

		G2G_ResLoginToken tokenRes;

		DNServerProxyHelper* server = GetGlobalServer()->GetSSock();
		unsigned int smsgId = server->GetMsgId();
		
		// pack data
		string binData;
		binData.resize(tokenReq.ByteSizeLong());
		tokenReq.SerializeToArray(binData.data(), (int)binData.size());
		MessagePack(smsgId, MsgDeal::Req, tokenReq.GetDescriptor()->full_name().c_str(), binData);
		
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
		response.set_state_code(1);
	}

	string binData;
	binData.resize(response.ByteSize());
	response.SerializeToArray(binData.data(), binData.size());

	MessagePack(msgId, MsgDeal::Res, nullptr, binData);
	
	channel->write(binData);

	co_return;
}