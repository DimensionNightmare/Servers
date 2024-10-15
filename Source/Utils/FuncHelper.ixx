module;
#include "StdMacro.h"
export module FuncHelper;

export import MessagePack;
import ThirdParty.Libhv;
import Logger;

export void MessagePackAndSend(uint32_t msgId, MsgDeal deal, const char* pbName, string& data, const SocketChannelPtr& channel)
{
	MessagePack(msgId, deal, pbName, data);
	channel->write(data);

	DNPrint(0, LoggerLevel::Debug, "{} Send type={} With Mid:{}, Mess:{}", channel->peeraddr().c_str(), (int)deal, msgId, pbName ? pbName : "");
}