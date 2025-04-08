module;
#include "StdMacro.h"
export module FuncHelper;

import MessagePack;
import ThirdParty.Libhv;
import Logger;

export void MessagePackAndSend(uint32_t msgId, EMMsgDeal deal, const char* pbName, std::string& data, const SocketChannelPtr& channel)
{
	MessagePack(msgId, deal, pbName, data);
	channel->write(data);

	DNPrint(ELogLevel_Debug, "%s Send type=%d With Mid:%u, Mess:%s", channel->peeraddr().c_str(), (int)deal, msgId, pbName ? pbName : "");
}