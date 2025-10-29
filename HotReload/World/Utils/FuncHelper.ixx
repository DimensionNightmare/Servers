export module FuncHelper;

import MessagePack;
import ThirdParty.Libhv;
import StrUtils;
import Logger;
import std.compat;

export 
{
	void MessagePackAndSend(uint32_t msgId, EMMsgDeal deal, std::string& data, SocketChannel::Ptr channel)
	{
		MessagePack(msgId, deal, 0, data);

		channel->write(data);

		LoggerPrint::Log(channel, ELogLevel_Debug, "{} Send type={} With Mid:{}", channel->peeraddr().c_str(), EnumName(deal), msgId);
	}

	void MessagePackAndSend(uint32_t msgId, EMMsgDeal deal, const std::string& pbName, std::string& data, SocketChannel::Ptr channel)
	{
		MessagePack(msgId, deal, DoStringHash(pbName), data);
		channel->write(data);

		LoggerPrint::Log(channel, ELogLevel_Debug, "{} Send type={} With Mid:{}, Mess:{}", channel->peeraddr().c_str(), EnumName(deal), msgId, pbName);
	}

	void MessagePackAndSend(uint32_t msgId, EMMsgDeal deal, const std::string& pbName, const std::string& data, SocketChannel::Ptr channel)
	{
		std::string msgData = data;
		MessagePack(msgId, deal, DoStringHash(pbName), msgData);
		channel->write(msgData);

		LoggerPrint::Log(channel, ELogLevel_Debug, "{} Send type={} With Mid:{}, Mess:{}", channel->peeraddr().c_str(), EnumName(deal), msgId, pbName);
	}

}
