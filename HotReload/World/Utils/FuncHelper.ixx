export module FuncHelper;

import MessagePack;
import ThirdParty.Libhv;
import StrUtils;
import Logger;
import std.compat;

export class FinalExecute
{
public:
	FinalExecute(std::function<void()> func):mFunc(func)
	{

	}

	~FinalExecute()
	{
		mFunc();
	}

private:
	std::function<void()> mFunc;
};

export void MessagePackAndSend(uint32_t msgId, EMMsgDeal deal, std::string& data, SocketChannel::CVPtr channel)
{
	MessagePack(msgId, deal, 0, data);

	channel->write(data);

	if(LoggerPrint::Ptr logger = channel->GetWorld()->GetSystem<LoggerPrint>(EMSystemType::LoggerPrint))
	{
		logger->Record(ELogLevel_Debug, "{} Send type={} With Mid:{}", channel->peeraddr().c_str(), (int)deal, msgId);
	} 
}

export void MessagePackAndSend(uint32_t msgId, EMMsgDeal deal, const std::string& pbName, std::string& data, SocketChannel::CVPtr channel)
{
	MessagePack(msgId, deal, DoStringHash(pbName), data);
	channel->write(data);

	if(LoggerPrint::Ptr logger = channel->GetWorld()->GetSystem<LoggerPrint>(EMSystemType::LoggerPrint))
	{
		logger->Record(ELogLevel_Debug, "{} Send type={} With Mid:{}, Mess:{}", channel->peeraddr().c_str(), (int)deal, msgId, pbName);
	}
}

export void MessagePackAndSend(uint32_t msgId, EMMsgDeal deal, const std::string& pbName, const std::string& data, SocketChannel::CVPtr channel)
{
	std::string msgData = data;
	MessagePack(msgId, deal, DoStringHash(pbName), msgData);
	channel->write(msgData);

	if(LoggerPrint::Ptr logger = channel->GetWorld()->GetSystem<LoggerPrint>(EMSystemType::LoggerPrint))
	{
		logger->Record(ELogLevel_Debug, "{} Send type={} With Mid:{}, Mess:{}", channel->peeraddr().c_str(), (int)deal, msgId, pbName);
	}
}
