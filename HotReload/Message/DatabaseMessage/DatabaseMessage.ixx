module;
#include <functional>
#include <cstdint>
#include "hv/Channel.h"

#include "StdMacro.h"
#include "Common/Common.pb.h"
#include "Server/S_Common.pb.h"
#include "Server/S_Dedicated.pb.h"
export module DatabaseMessage;

export import :DatabaseCommon;
import :DatabaseGate;
import Logger;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg;

export class DatabaseMessageHandle
{
public:
	static void MsgHandle(const SocketChannelPtr& channel, uint32_t msgId, size_t msgHashId, const string& msgData);
	static void MsgRetHandle(const SocketChannelPtr& channel, size_t msgHashId, const string& msgData);
	static void RegMsgHandle();
public:
	inline static unordered_map<size_t, pair<const Message*, function<void(SocketChannelPtr, uint32_t, Message*)>>> MHandleMap;
	inline static unordered_map<size_t, pair<const Message*, function<void(SocketChannelPtr, Message*)>>> MHandleRetMap;
};



void DatabaseMessageHandle::MsgHandle(const SocketChannelPtr& channel, uint32_t msgId, size_t msgHashId, const string& msgData)
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
			DNPrint(ErrCode_MsgParse, LoggerLevel::Error, nullptr);
		}

		delete message;
	}
	else
	{
		DNPrint(ErrCode_MsgHandleFind, LoggerLevel::Error, nullptr);
	}
}

void DatabaseMessageHandle::MsgRetHandle(const SocketChannelPtr& channel, size_t msgHashId, const string& msgData)
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
			DNPrint(ErrCode_MsgParse, LoggerLevel::Error, nullptr);
		}

		delete message;
	}
	else
	{
		DNPrint(ErrCode_MsgHandleFind, LoggerLevel::Error, nullptr);
	}
}

void DatabaseMessageHandle::RegMsgHandle()
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

	MSG_MAPPING(MHandleMap, d2D_ReqLoadData, Exe_ReqLoadData);
	MSG_MAPPING(MHandleMap, d2D_ReqSaveData, Exe_ReqSaveData);

	MSG_MAPPING(MHandleRetMap, COM_RetChangeCtlSrv, Exe_RetChangeCtlSrv);
}