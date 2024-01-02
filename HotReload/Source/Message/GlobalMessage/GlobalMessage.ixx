module;
#include "GlobalControl.pb.h"
#include "hv/Channel.h"

#include <map>
#include <functional>
export module GlobalMessage;

export import GlobalControl;

using namespace std;
using namespace hv;
using namespace google::protobuf;

export class GlobalMessageHandle
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

void GlobalMessageHandle::MsgHandle(const SocketChannelPtr &channel, unsigned int msgId, size_t msgHashId, const string& msgData)
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

void GlobalMessageHandle::RegMsgHandle()
{
	
}