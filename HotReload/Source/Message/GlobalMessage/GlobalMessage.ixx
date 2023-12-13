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
	static void MsgHandle(const SocketChannelPtr &channel, unsigned int msgId, string name, Message *msg);
	static void RegMsgHandle();
public:
	inline static map<string, function<void(const SocketChannelPtr &, unsigned int, Message *)>> MHandleMap;
};

module :private;

void GlobalMessageHandle::MsgHandle(const SocketChannelPtr &channel, unsigned int msgId, string name, Message *msg)
{
	if (MHandleMap.contains(name))
	{
		MHandleMap[name](channel, msgId, msg);
	}
}

void GlobalMessageHandle::RegMsgHandle()
{
	
}