module;
#include "GlobalControl.pb.h"

#include "hv/Channel.h"

#include <map>
#include <functional>
export module ControlMessage;

export import ControlGlobal;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg::GlobalControl;

export class ControlMessageHandle
{
public:
	static void MsgHandle(const SocketChannelPtr &channel, unsigned int msgId, size_t msgHashId, const string& msgData);
	static void RegMsgHandle();
public:
	inline static map<
		size_t, 
		pair<
			const Message*, 
			function<void(const SocketChannelPtr &, unsigned int, Message *)> 
		> 
	> MHandleMap;
};

module :private;

void ControlMessageHandle::MsgHandle(const SocketChannelPtr &channel, unsigned int msgId, size_t msgHashId, const string& msgData)
{
	if (MHandleMap.contains(msgHashId))
	{
		auto& handle = MHandleMap[msgHashId];
		auto message = handle.first->New();
		if(message->ParseFromArray(msgData.data(), msgData.length()))
			handle.second(channel, msgId, message);
		
		delete message;
	}
}

void ControlMessageHandle::RegMsgHandle()
{
	std::hash<string> hashStr;

	const Message* msg = G2C_RegistSrv::internal_default_instance();
	MHandleMap.emplace( hashStr(msg->GetDescriptor()->full_name()), make_pair(msg, &Msg_RegistSrv));
}