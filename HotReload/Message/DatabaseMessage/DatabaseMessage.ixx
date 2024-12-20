module;
#include "StdMacro.h"
export module DatabaseMessage;

export import :DatabaseCommon;
import :DatabaseGate;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;

export class DatabaseMessageHandle
{

public:

	static void MsgHandle(const SocketChannelPtr& channel, uint32_t msgId, size_t msgHashId, const string& msgData)
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
				DNPrint(0, EMLoggerLevel::Debug, e.what());
			}
		}
		else
		{
			DNPrint(ErrCode::ErrCode_MsgHandleFind, EMLoggerLevel::Error, nullptr);
		}
	}

	static void MsgRetHandle(const SocketChannelPtr& channel, size_t msgHashId, const string& msgData)
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
				DNPrint(0, EMLoggerLevel::Debug, e.what());
			}
		}
		else
		{
			DNPrint(ErrCode::ErrCode_MsgHandleFind, EMLoggerLevel::Error, nullptr);
		}
	}

	static void RegMsgHandle()
	{
#ifdef _WIN32
	#define MSG_MAPPING(map, msg, func) \
		map.emplace(std::hash<string>::_Do_hash(msg::GetDescriptor()->full_name()), \
		make_pair(msg::internal_default_instance(), &DatabaseMessage::func))
#elif __unix__
	#define MSG_MAPPING(map, msg, func) \
		map.emplace(std::hash<string>{}(msg::GetDescriptor()->full_name()), \
		make_pair(msg::internal_default_instance(), &DatabaseMessage::func))
#endif

		MSG_MAPPING(MHandleMap, L2D_ReqLoadData, Exe_ReqLoadData);
		MSG_MAPPING(MHandleMap, L2D_ReqSaveData, Exe_ReqSaveData);

		MSG_MAPPING(MHandleRetMap, COM_RetChangeCtlSrv, Exe_RetChangeCtlSrv);
	}
public:

	inline static unordered_map<size_t, pair<const Message*, function<void(SocketChannelPtr, uint32_t, string)>>> MHandleMap;

	inline static unordered_map<size_t, pair<const Message*, function<void(SocketChannelPtr, string)>>> MHandleRetMap;
};
