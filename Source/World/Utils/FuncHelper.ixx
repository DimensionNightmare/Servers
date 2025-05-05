module;
export module FuncHelper;

import MessagePack;
import ThirdParty.Libhv;
import Logger;
import std.compat;

export void MessagePackAndSend(uint32_t msgId, EMMsgDeal deal, const std::string& pbName, std::string& data, const hv::SocketChannelPtr& channel)
{
	MessagePack(msgId, deal, pbName, data);
	channel->write(data);

	SPidLogger.Record(ELogLevel_Debug, "{} Send type={} With Mid:{}, Mess:{}", channel->peeraddr().c_str(), (int)deal, msgId, pbName);
}