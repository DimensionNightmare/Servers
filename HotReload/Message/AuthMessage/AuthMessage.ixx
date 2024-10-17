module;
#include "StdMacro.h"
export module AuthMessage;

export import :AuthCommon;
import ApiManager;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import Logger;

export class AuthMessageHandle
{

public:

	static void MsgHandle(SocketChannelPtr channel, uint32_t msgId, size_t msgHashId, const string& msgData)
	{
		if (MHandleMap.contains(msgHashId))
		{
			auto& handle = MHandleMap[msgHashId];

			try
			{
				handle.second(channel, msgId, msgData);
			}
			catch (const exception& e)
			{
				DNPrint(0, LoggerLevel::Debug, e.what());
			}

		}
		else
		{
			DNPrint(ErrCode::ErrCode_MsgHandleFind, LoggerLevel::Error, nullptr);
		}
	}
	
	static void MsgRetHandle(SocketChannelPtr channel, size_t msgHashId, const string& msgData)
	{
		if (MHandleRetMap.contains(msgHashId))
		{
			auto& handle = MHandleRetMap[msgHashId];
			try
			{
				handle.second(channel, msgData);
			}
			catch (const exception& e)
			{
				DNPrint(0, LoggerLevel::Debug, e.what());
			}
		}
		else
		{
			DNPrint(ErrCode::ErrCode_MsgHandleFind, LoggerLevel::Error, nullptr);
		}
	}

	static void RegMsgHandle()
	{

	}

	static void RegApiHandle(HttpService* service)
	{
		service->Static("/", "./");

		ApiInit(service);
	}
public:

	inline static unordered_map<size_t, pair<const Message*, function<void(SocketChannelPtr, uint32_t, string)>>> MHandleMap;

	inline static unordered_map<size_t, pair<const Message*, function<void(SocketChannelPtr, string)>>> MHandleRetMap;
};
