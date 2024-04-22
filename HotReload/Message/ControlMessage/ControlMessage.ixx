module;
#include <map>
#include <functional>
#include "hv/Channel.h"

#include "StdAfx.h"
#include "Server/S_Common.pb.h"
#include "Server/S_Auth.pb.h"
export module ControlMessage;

export import :ControlGlobal;
import :ControlCommon;
import :ControlAuth;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg::S_Common;
using namespace GMsg::S_Auth;

export class ControlMessageHandle
{
public:
	static void MsgHandle(const SocketChannelPtr &channel, uint32_t msgId, size_t msgHashId, const string& msgData);
	static void MsgRetHandle(const SocketChannelPtr &channel, uint32_t msgId, size_t msgHashId, const string& msgData);
	static void RegMsgHandle();
public:
	inline static map<
		size_t, 
		pair<
			const Message*, 
			function<void(const SocketChannelPtr &, uint32_t, Message *)> 
		> 
	> MHandleMap;

	inline static map<
		size_t, 
		pair<
			const Message*, 
			function<void(const SocketChannelPtr &, uint32_t, Message *)> 
		> 
	> MHandleRetMap;
};



void ControlMessageHandle::MsgHandle(const SocketChannelPtr &channel, uint32_t msgId, size_t msgHashId, const string& msgData)
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

void ControlMessageHandle::MsgRetHandle(const SocketChannelPtr &channel, uint32_t msgId, size_t msgHashId, const string &msgData)
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

void ControlMessageHandle::RegMsgHandle()
{
	std::hash<string> hashStr;

	MSG_MAPPING(MHandleMap, COM_ReqRegistSrv, &Msg_ReqRegistSrv);
	MSG_MAPPING(MHandleMap, A2G_ReqAuthAccount, &Msg_ReqAuthAccount);
	MSG_MAPPING(MHandleRetMap, COM_RetHeartbeat, &Exe_RetHeartbeat);
}
