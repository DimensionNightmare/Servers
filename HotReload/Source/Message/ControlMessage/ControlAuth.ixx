module;
#include "GlobalAuth.pb.h"
#include "GlobalGate.pb.h"
#include "GlobalControl.pb.h"
#include "hv/Channel.h"

#include <coroutine>
export module ControlMessage:ControlAuth;

import DNTask;
import MessagePack;
import ControlServerHelper;
import ServerEntityHelper;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg::GlobalAuth;
using namespace GMsg::GlobalGate;

export DNTaskVoid Exe_ReqAuthAccount(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	A2G_ReqAuthAccount* requset = (A2G_ReqAuthAccount*)msg;
	G2A_ResAuthAccount response;

	ServerEntityHelper* entity = nullptr;
	list<ServerEntity*>& servList = GetControlServer()->GetEntityManager()->GetEntityByList(ServerType::GlobalServer);
	for(ServerEntity* it : servList)
	{
		ServerEntityHelper* castEntity = static_cast<ServerEntityHelper*>(it);

		if(castEntity->GetChild()->TimerId())
		{
			continue;
		}

		if(!entity)
		{
			entity = castEntity;
			continue;
		}

		if(castEntity->GetConnNum() < entity->GetConnNum())
		{
			entity = castEntity;
		}
	}

	string binData;

	if(!entity)
	{
		response.set_state_code(1);
	}
	else
	{
		// response.set_state_code(0);

		// message change to global
		auto dataChannel = [&response]()->DNTask<Message*>
		{
			co_return &response;
		}();

		auto server = GetControlServer()->GetSSock();
		unsigned int smsgId = server->GetMsgId();
		server->AddMsg(smsgId, &dataChannel);

		binData.resize(requset->ByteSize());
		requset->SerializeToArray(binData.data(), binData.size());
		MessagePack(smsgId, MsgDeal::Req, requset->GetDescriptor()->full_name().c_str(), binData);
		
		// wait data parse
		entity->GetChild()->GetSock()->write(binData);
		co_await dataChannel;

	}

	
	binData.resize(response.ByteSize());
	response.SerializeToArray(binData.data(), binData.size());

	MessagePack(msgId, MsgDeal::Res, nullptr, binData);
	
	channel->write(binData);

	co_return;
}