module;
export module AuthMessage;

export import :AuthCommon;
import ApiManager;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import Logger;
import std.compat;
import StrUtils;

export class AuthMessageHandle
{

public:

	static void MsgHandle(hv::SocketChannelPtr channel, uint32_t msgId, size_t msgHashId, const std::string& msgData)
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
				LoggerPrint()(ELogLevel_Debug, e.what());
			}

		}
		else
		{
			LoggerPrint()(EL10nCode_MsgHandleFind);
		}
	}
	
	static void MsgRetHandle(hv::SocketChannelPtr channel, size_t msgHashId, const std::string& msgData)
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
				LoggerPrint()(ELogLevel_Debug, e.what());
			}
		}
		else
		{
			LoggerPrint()(EL10nCode_MsgHandleFind);
		}
	}

	static void RegMsgHandle()
	{

	}

	static void RegApiHandle(hv::HttpService* service)
	{
		service->Static("/", "./");

		ApiInit(service);
	}
public:

	inline static std::unordered_map<size_t, std::pair<const Message*, std::function<void(hv::SocketChannelPtr, uint32_t, std::string)>>> MHandleMap;

	inline static std::unordered_map<size_t, std::pair<const Message*, std::function<void(hv::SocketChannelPtr, std::string)>>> MHandleRetMap;
};
