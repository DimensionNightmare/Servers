export module FuncHelper;

import MessagePack;
import ThirdParty.Libhv;
import StrUtils;
import Logger;
import std.compat;
import ThirdParty.Protobuf;

export 
{

	void MessagePackAndSend(uint32_t msgId, EMMsgDeal deal, Message* message, SocketChannel::Ptr channel)
	{
		size_t hash = 0;
		std::string msgData;
		std::string msgName;

		if(deal != EMMsgDeal::Res)
		{
			msgName = message->GetDescriptor()->full_name();
			hash = DoStringHash(msgName);
		}

		message->SerializeToString(&msgData);

		MessagePack(msgId, deal, hash, msgData);
		channel->write(msgData);

		LoggerPrint::Log(channel, ELogLevel_Debug, "{} Send type={} With Mid:{}, Mess:{}", channel->peeraddr(), EnumName(deal), msgId, msgName);
	}

}
