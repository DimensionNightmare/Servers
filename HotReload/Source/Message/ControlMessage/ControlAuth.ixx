module;
#include "AuthControl.pb.h"
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
using namespace GMsg::AuthControl;

export DNTaskVoid Exe_AuthAccount(const SocketChannelPtr &channel, unsigned int msgId, Message *msg)
{
	A2C_AuthAccount* requset = (A2C_AuthAccount*)msg;
	C2A_AuthAccount response;

	ServerEntityHelper* entity = nullptr;
	auto& servList = GetControlServer()->GetEntityManager()->GetEntity(ServerType::GlobalServer);
	for(auto& it : servList)
	{
		auto castEntity = static_cast<ServerEntityHelper*>(it);

		if(castEntity->GetChild()->GetTimerId())
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

		auto sock = GetControlServer()->GetSSock();
		auto smsgId = sock->GetMsgId();
		sock->AddMsg(smsgId, (DNTask<void*>*)&dataChannel);

		binData.resize(requset->ByteSize());
		requset->SerializeToArray(binData.data(), binData.size());
		MessagePack(smsgId, MsgDeal::Req, requset->GetDescriptor()->full_name(), binData);
		
		// wait data parse
		entity->GetChild()->GetSock()->write(binData);
		co_await dataChannel;

	}

	
	binData.resize(response.ByteSize());
	response.SerializeToArray(binData.data(), binData.size());

	MessagePack(msgId, MsgDeal::Res, "", binData);
	
	// GetControlServer()->GetSSock()->loop()->setTimeout(10000, [channel, binData](TimerID timerID) 
	// {
        channel->write(binData);
    // });

	co_return;
}