module;
#include "StdMacro.h"
export module LogicMessage;

export import :LogicCommon;
import :LogicGate;
import :LogicRedirect;
import :LogicDedicated;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;

export class LogicMessageHandle
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

	static void MsgRedirectHandle(const SocketChannelPtr& channel, uint32_t msgId, size_t msgHashId, const string& msgData)
	{
		if (MHandleRedirectMap.contains(msgHashId))
		{
			auto& handle = MHandleRedirectMap[msgHashId];
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

	static void RegMsgHandle()
	{
#ifdef _WIN32
	#define MSG_MAPPING(map, msg, func) \
		map.emplace(std::hash<string>::_Do_hash(msg::GetDescriptor()->full_name()), \
		make_pair(msg::internal_default_instance(), &LogicMessage::func))
#elif __unix__
	#define MSG_MAPPING(map, msg, func) \
		map.emplace(std::hash<string>{}(msg::GetDescriptor()->full_name()), \
		make_pair(msg::internal_default_instance(), &LogicMessage::func))
#endif

		MSG_MAPPING(MHandleMap, d2L_ReqRegistSrv, Msg_ReqRegistSrv);
		MSG_MAPPING(MHandleMap, d2L_ReqLoadEntityData, Msg_ReqLoadEntityData);

		MSG_MAPPING(MHandleRetMap, COM_RetChangeCtlSrv, Exe_RetChangeCtlSrv);
		MSG_MAPPING(MHandleRetMap, COM_RetHeartbeat, Exe_RetHeartbeat);
		MSG_MAPPING(MHandleRetMap, g2L_RetProxyOffline, Exe_RetProxyOffline);
		MSG_MAPPING(MHandleRetMap, d2L_ReqSaveEntityData, Msg_ReqSaveEntityData);


		MSG_MAPPING(MHandleRedirectMap, S2C_RetAccountReplace, Exe_RetAccountReplace);
		MSG_MAPPING(MHandleRedirectMap, C2S_ReqAuthToken, Msg_ReqClientLogin);
	}
public:

	inline static unordered_map<size_t, pair<const Message*, function<void(SocketChannelPtr, uint32_t, string)>>> MHandleMap;

	inline static unordered_map<size_t, pair<const Message*, function<void(SocketChannelPtr, string)>>> MHandleRetMap;

	inline static unordered_map<size_t, pair<const Message*, function<void(SocketChannelPtr, uint32_t, string)>>> MHandleRedirectMap;
};
