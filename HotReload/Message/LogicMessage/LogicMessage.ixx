module;
#include <map>
#include <functional>
#include <cstdint>
#include "hv/Channel.h"

#include "StdAfx.h"
#include "Server/S_Common.pb.h"
#include "Server/S_Gate_Logic.pb.h"
export module LogicMessage;

export import :LogicCommon;
import :LogicGate;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg;
using namespace GMsg;

export class LogicMessageHandle
{
public:
	static void MsgHandle(SocketChannelPtr channel, uint32_t msgId, size_t msgHashId, const string& msgData);
	static void MsgRetHandle(SocketChannelPtr channel, uint32_t msgId, size_t msgHashId, const string& msgData);
	static void RegMsgHandle();
public:
	inline static map<
		size_t, 
		pair<
			const Message*, 
			function<void(SocketChannelPtr, uint32_t, Message *)> 
		> 
	> MHandleMap;

	inline static map<
		size_t, 
		pair<
			const Message*, 
			function<void(SocketChannelPtr, uint32_t, Message *)> 
		> 
	> MHandleRetMap;
};



void LogicMessageHandle::MsgHandle(SocketChannelPtr channel, uint32_t msgId, size_t msgHashId, const string& msgData)
{
	if (MHandleMap.contains(msgHashId))
	{
		auto& handle = MHandleMap[msgHashId];
		Message* message = handle.first->New();
		if(message->ParseFromArray(msgData.data(), msgData.length()))
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

void LogicMessageHandle::MsgRetHandle(SocketChannelPtr channel, uint32_t msgId, size_t msgHashId, const string &msgData)
{
	if (MHandleRetMap.contains(msgHashId))
	{
		auto& handle = MHandleRetMap[msgHashId];
		Message* message = handle.first->New();
		if(message->ParseFromArray(msgData.data(), msgData.length()))
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

void LogicMessageHandle::RegMsgHandle()
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

	MSG_MAPPING(MHandleRetMap, COM_RetChangeCtlSrv, Exe_RetChangeCtlSrv);
	MSG_MAPPING(MHandleMap, COM_ReqRegistSrv, Msg_ReqRegistSrv);
	MSG_MAPPING(MHandleRetMap, COM_RetHeartbeat, Exe_RetHeartbeat);
	MSG_MAPPING(MHandleMap, G2L_ReqClientLogin, Msg_ReqClientLogin);
	MSG_MAPPING(MHandleRetMap, G2L_RetAccountReplace, Exe_RetAccountReplace);
}