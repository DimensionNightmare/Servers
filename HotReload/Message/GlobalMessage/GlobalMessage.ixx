module;
#include <map>
#include <functional>
#include <cstdint>
#include "hv/Channel.h"

#include "StdAfx.h"
#include "Server/S_Control_Global.pb.h"
#include "Server/S_Common.pb.h"
#include "Server/S_Global_Gate.pb.h"
export module GlobalMessage;

export import :GlobalCommon;
import :GlobalControl;
import :GlobalGate;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg;

export class GlobalMessageHandle
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



void GlobalMessageHandle::MsgHandle(SocketChannelPtr channel, uint32_t msgId, size_t msgHashId, const string& msgData)
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

void GlobalMessageHandle::MsgRetHandle(SocketChannelPtr channel, uint32_t msgId, size_t msgHashId, const string &msgData)
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
	MSG_MAPPING(MHandleMap, C2G_ReqAuthAccount, Msg_ReqAuthAccount);
	MSG_MAPPING(MHandleRetMap, g2G_RetRegistSrv, Exe_RetRegistSrv);
	MSG_MAPPING(MHandleRetMap, COM_RetHeartbeat, Exe_RetHeartbeat);
}