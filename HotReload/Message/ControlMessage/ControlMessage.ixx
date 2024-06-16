module;
#include <functional>
#include <cstdint>
#include <string>

#include "StdMacro.h"
export module ControlMessage;

export import :ControlGlobal;
import :ControlCommon;
import :ControlAuth;
import :ControlRedirect;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;

export class ControlMessageHandle
{
public:
	static void MsgHandle(const SocketChannelPtr& channel, uint32_t msgId, size_t msgHashId, const string& msgData);
	static void MsgRetHandle(const SocketChannelPtr& channel, size_t msgHashId, const string& msgData);
	static void MsgRedirectHandle(const SocketChannelPtr& channel, uint32_t msgId, size_t msgHashId, const string& msgData);
	static void RegMsgHandle();
public:
	inline static unordered_map<size_t, pair<const Message*, function<void(SocketChannelPtr, uint32_t, Message*)>>> MHandleMap;
	inline static unordered_map<size_t, pair<const Message*, function<void(SocketChannelPtr, Message*)>>> MHandleRetMap;
	inline static unordered_map<size_t, pair<const Message*, function<void(SocketChannelPtr, uint32_t, Message*)>>> MHandleRedirectMap;
};



void ControlMessageHandle::MsgHandle(const SocketChannelPtr& channel, uint32_t msgId, size_t msgHashId, const string& msgData)
{
	if (MHandleMap.contains(msgHashId))
	{
		auto& handle = MHandleMap[msgHashId];
		Message* message = handle.first->New();
		if (message->ParseFromString(msgData))
		{
			try
			{
				handle.second(channel, msgId, message);
			}
			catch (const exception& e)
			{
				DNPrint(0, LoggerLevel::Debug, e.what());
			}
		}
		else
		{
			DNPrint(ErrCode::ErrCode_MsgParse, LoggerLevel::Error, nullptr);
		}

		delete message;
	}
	else
	{
		DNPrint(ErrCode::ErrCode_MsgHandleFind, LoggerLevel::Error, nullptr);
	}
}

void ControlMessageHandle::MsgRetHandle(const SocketChannelPtr& channel, size_t msgHashId, const string& msgData)
{
	if (MHandleRetMap.contains(msgHashId))
	{
		auto& handle = MHandleRetMap[msgHashId];
		Message* message = handle.first->New();
		if (message->ParseFromString(msgData))
		{
			try
			{
				handle.second(channel, message);
			}
			catch (const exception& e)
			{
				DNPrint(0, LoggerLevel::Debug, e.what());
			}
		}
		else
		{
			DNPrint(ErrCode::ErrCode_MsgParse, LoggerLevel::Error, nullptr);
		}

		delete message;
	}
	else
	{
		DNPrint(ErrCode::ErrCode_MsgHandleFind, LoggerLevel::Error, nullptr);
	}
}

void ControlMessageHandle::MsgRedirectHandle(const SocketChannelPtr& channel, uint32_t msgId, size_t msgHashId, const string& msgData)
{
	if (MHandleRedirectMap.contains(msgHashId))
	{
		auto& handle = MHandleRedirectMap[msgHashId];
		Message* message = handle.first->New();
		if (message->ParseFromString(msgData))
		{
			try
			{
				handle.second(channel, msgId, message);
			}
			catch (const exception& e)
			{
				DNPrint(0, LoggerLevel::Debug, e.what());
			}
		}
		else
		{
			DNPrint(ErrCode::ErrCode_MsgParse, LoggerLevel::Error, nullptr);
		}

		delete message;
	}
	else
	{
		DNPrint(ErrCode::ErrCode_MsgHandleFind, LoggerLevel::Error, nullptr);
	}
}

void ControlMessageHandle::RegMsgHandle()
{
#ifdef _WIN32
#define MSG_MAPPING(map, msg, func) \
		map.emplace(std::hash<string>::_Do_hash(msg::GetDescriptor()->full_name()), \
		make_pair(msg::internal_default_instance(), &ControlMessage::func))
#elif __unix__
#define MSG_MAPPING(map, msg, func) \
		map.emplace(std::hash<string>{}(msg::GetDescriptor()->full_name()), \
		make_pair(msg::internal_default_instance(), &ControlMessage::func))
#endif

	MSG_MAPPING(MHandleMap, COM_ReqRegistSrv, Msg_ReqRegistSrv);

	MSG_MAPPING(MHandleRetMap, COM_RetHeartbeat, Exe_RetHeartbeat);

	MSG_MAPPING(MHandleRedirectMap, A2g_ReqAuthAccount, Msg_ReqAuthAccount);
}
