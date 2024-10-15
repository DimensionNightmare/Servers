module;
#include "StdMacro.h"
export module GlobalMessage;

export import :GlobalCommon;
import :GlobalControl;
import :GlobalGate;
import :GlobalRedirect;
import Logger;
import Config.Server;
import ThirdParty.Libhv;
import ThirdParty.PbGen;

export class GlobalMessageHandle
{
public:
	static void MsgHandle(const SocketChannelPtr& channel, uint32_t msgId, size_t msgHashId, const string& msgData);
	static void MsgRetHandle(const SocketChannelPtr& channel, size_t msgHashId, const string& msgData);
	static void MsgRedirectHandle(const SocketChannelPtr& channel, uint32_t msgId, size_t msgHashId, const string& msgData);
	static void RegMsgHandle();
public:
	inline static unordered_map<size_t, pair<const Message*, function<void(SocketChannelPtr, uint32_t, string)>>> MHandleMap;
	inline static unordered_map<size_t, pair<const Message*, function<void(SocketChannelPtr, string)>>> MHandleRetMap;
	inline static unordered_map<size_t, pair<const Message*, function<void(SocketChannelPtr, uint32_t, string)>>> MHandleRedirectMap;
};



void GlobalMessageHandle::MsgHandle(const SocketChannelPtr& channel, uint32_t msgId, size_t msgHashId, const string& msgData)
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

void GlobalMessageHandle::MsgRetHandle(const SocketChannelPtr& channel, size_t msgHashId, const string& msgData)
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

void GlobalMessageHandle::MsgRedirectHandle(const SocketChannelPtr& channel, uint32_t msgId, size_t msgHashId, const string& msgData)
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
			DNPrint(0, LoggerLevel::Debug, e.what());
		}
	}
	else
	{
		DNPrint(ErrCode::ErrCode_MsgHandleFind, LoggerLevel::Error, nullptr);
	}
}

void GlobalMessageHandle::RegMsgHandle()
{
#ifdef _WIN32
#define MSG_MAPPING(map, msg, func) \
		map.emplace(std::hash<string>::_Do_hash(msg::GetDescriptor()->full_name()), \
		make_pair(msg::internal_default_instance(), &GlobalMessage::func))
#elif __unix__
#define MSG_MAPPING(map, msg, func) \
		map.emplace(std::hash<string>{}(msg::GetDescriptor()->full_name()), \
		make_pair(msg::internal_default_instance(), &GlobalMessage::func))
#endif

	MSG_MAPPING(MHandleMap, COM_ReqRegistSrv, Msg_ReqRegistSrv);

	MSG_MAPPING(MHandleRetMap, g2G_RetRegistSrv, Exe_RetRegistSrv);
	MSG_MAPPING(MHandleRetMap, COM_RetHeartbeat, Exe_RetHeartbeat);
	MSG_MAPPING(MHandleRetMap, g2G_RetRegistChild, Exe_RetRegistChild);

	MSG_MAPPING(MHandleRedirectMap, A2g_ReqAuthAccount, Msg_ReqAuthAccount);
}