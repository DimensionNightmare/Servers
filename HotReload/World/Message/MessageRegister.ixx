module;

export module MessageRegister;

import Logger;
import ThirdParty.Protobuf;
import ThirdParty.Libhv;
import DNServer;

export class MessageRegister
{

public:

	void MsgHandle(DNSocketChannel::CVPtr channel, uint32_t msgId, size_t msgHashId, const std::string& msgData)
	{
		if (MHandleMap.contains(msgHashId))
		{
			auto& handle = MHandleMap[msgHashId];
			try
			{
				handle.second(channel, msgId, msgData);
			}
			catch (const std::exception& e)
			{
				if(World::CVPtr pWorld = channel->GetWorld())
				{
					pWorld->GetSystem<LoggerPrint>(EMSystemType::LoggerPrint)->Record(ELogLevel_Debug, "{}", e.what());
				}
			}
		}
		else
		{
			if(World::CVPtr pWorld = channel->GetWorld())
			{
				pWorld->GetSystem<LoggerPrint>(EMSystemType::LoggerPrint)->Record(EL10nCode_MsgHandleFind);
			}
		}
	}

	void MsgRetHandle(DNSocketChannel::CVPtr channel, size_t msgHashId, const std::string& msgData)
	{
		if (MHandleRetMap.contains(msgHashId))
		{
			auto& handle = MHandleRetMap[msgHashId];
			try
			{
				handle.second(channel, msgData);
			}
			catch (const std::exception& e)
			{
				if(World::CVPtr pWorld = channel->GetWorld())
				{
					pWorld->GetSystem<LoggerPrint>(EMSystemType::LoggerPrint)->Record(ELogLevel_Debug, "{}", e.what());
				}
			}
		}
		else
		{
			if(World::CVPtr pWorld = channel->GetWorld())
			{
				pWorld->GetSystem<LoggerPrint>(EMSystemType::LoggerPrint)->Record(EL10nCode_MsgHandleFind);
			}
		}
	}

	void MsgRedirectHandle(DNSocketChannel::CVPtr channel, uint32_t msgId, size_t msgHashId, const std::string& msgData)
	{
		if (MHandleRedirectMap.contains(msgHashId))
		{
			auto& handle = MHandleRedirectMap[msgHashId];
			try
			{
				handle.second(channel, msgId, msgData);
			}
			catch (const std::exception& e)
			{
				if(World::CVPtr pWorld = channel->GetWorld())
				{
					pWorld->GetSystem<LoggerPrint>(EMSystemType::LoggerPrint)->Record(ELogLevel_Debug, "{}", e.what());
				}
			}
		}
		else
		{
			if(World::CVPtr pWorld = channel->GetWorld())
			{
				pWorld->GetSystem<LoggerPrint>(EMSystemType::LoggerPrint)->Record(EL10nCode_MsgHandleFind);
			}
		}
	}

	virtual void RegMsgHandle() = 0;

	virtual std::function<void(DNServer::CVPtr)> GetClientRegistFunc() = 0;

	virtual void RegApiHandle(DNServer::CVPtr server){}

protected:
	std::unordered_map<size_t, std::pair<const Message*, std::function<void(DNSocketChannel::CVPtr, uint32_t, const std::string&)>>> MHandleMap;
	std::unordered_map<size_t, std::pair<const Message*, std::function<void(DNSocketChannel::CVPtr, const std::string&)>>> MHandleRetMap;
	std::unordered_map<size_t, std::pair<const Message*, std::function<void(DNSocketChannel::CVPtr, uint32_t, const std::string&)>>> MHandleRedirectMap;
};