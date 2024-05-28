module;
#include <functional>
#include <cstdint>
#include "hv/Channel.h"

#include "StdMacro.h"
#include "Common/Common.pb.h"
#include "Server/S_Common.pb.h"
#include "Server/S_Auth_Control.pb.h"
export module ControlMessage;

export import :ControlGlobal;
import :ControlCommon;
import :ControlAuth;
import Logger;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg;

export class ControlMessageHandle
{
public:
	static void MsgHandle(const SocketChannelPtr& channel, uint32_t msgId, size_t msgHashId, const string& msgData);
	static void MsgRetHandle(const SocketChannelPtr& channel, uint32_t msgId, size_t msgHashId, const string& msgData);
	static void RegMsgHandle();
public:
	inline static unordered_map<size_t, pair<const Message*, function<void(SocketChannelPtr, uint32_t, Message*)>>> MHandleMap;
	inline static unordered_map<size_t, pair<const Message*, function<void(SocketChannelPtr, uint32_t, Message*)>>> MHandleRetMap;
};



void ControlMessageHandle::MsgHandle(const SocketChannelPtr& channel, uint32_t msgId, size_t msgHashId, const string& msgData)
{
	if (MHandleMap.contains(msgHashId))
	{
		auto& handle = MHandleMap[msgHashId];
		Message* message = handle.first->New();
		if (message->ParseFromArray(msgData.data(), msgData.length()))
		{
			handle.second(channel, msgId, message);
		}
		else
		{
			DNPrint(ErrCode_MsgParse, LoggerLevel::Error, nullptr);
		}

		delete message;
	}
	else
	{
		DNPrint(ErrCode_MsgHandleFind, LoggerLevel::Error, nullptr);
	}
}

void ControlMessageHandle::MsgRetHandle(const SocketChannelPtr& channel, uint32_t msgId, size_t msgHashId, const string& msgData)
{
	if (MHandleRetMap.contains(msgHashId))
	{
		auto& handle = MHandleRetMap[msgHashId];
		Message* message = handle.first->New();
		if (message->ParseFromArray(msgData.data(), msgData.length()))
		{
			handle.second(channel, msgId, message);
		}
		else
		{
			DNPrint(ErrCode_MsgParse, LoggerLevel::Error, nullptr);
		}

		delete message;
	}
	else
	{
		DNPrint(ErrCode_MsgHandleFind, LoggerLevel::Error, nullptr);
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
	MSG_MAPPING(MHandleMap, A2C_ReqAuthAccount, Msg_ReqAuthAccount);
	MSG_MAPPING(MHandleRetMap, COM_RetHeartbeat, Exe_RetHeartbeat);
}
