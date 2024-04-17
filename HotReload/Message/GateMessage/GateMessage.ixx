module;
#include "StdAfx.h"
#include "S_Common.pb.h"
#include "S_Global.pb.h"
#include "C_Auth.pb.h"
#include "hv/Channel.h"

#include <map>
#include <functional>
export module GateMessage;

export import :GateCommon;
import :GateGlobal;
import :GateClient;


using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg::S_Common;
using namespace GMsg::S_Global;
using namespace GMsg::C_Auth;

export class GateMessageHandle
{
public:
	static void MsgHandle(const SocketChannelPtr &channel, unsigned int msgId, size_t msgHashId, const string& msgData);
	static void MsgRetHandle(const SocketChannelPtr &channel, unsigned int msgId, size_t msgHashId, const string& msgData);
	static void RegMsgHandle();
public:
	inline static map<
		size_t, 
		pair<
			const Message*, 
			function<void(const SocketChannelPtr &, unsigned int, Message *)> 
		> 
	> MHandleMap;

	inline static map<
		size_t, 
		pair<
			const Message*, 
			function<void(const SocketChannelPtr &, unsigned int, Message *)> 
		> 
	> MHandleRetMap;
};



void GateMessageHandle::MsgHandle(const SocketChannelPtr &channel, unsigned int msgId, size_t msgHashId, const string& msgData)
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
			DNPrint(14, LoggerLevel::Error, nullptr);
		}
		
		delete message;
	}
	else
	{
		DNPrint(15, LoggerLevel::Error, nullptr);
	}
}


void GateMessageHandle::MsgRetHandle(const SocketChannelPtr &channel, unsigned int msgId, size_t msgHashId, const string &msgData)
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
			DNPrint(14, LoggerLevel::Error, nullptr);
		}
		
		delete message;
	}
	else
	{
		DNPrint(15, LoggerLevel::Error, nullptr);
	}
}

void GateMessageHandle::RegMsgHandle()
{
	std::hash<string> hashStr;
	const Message* msg = nullptr;

	msg = COM_ReqRegistSrv::internal_default_instance();
	MHandleMap.emplace( hashStr(msg->GetDescriptor()->full_name()), make_pair(msg, &Msg_ReqRegistSrv));

	msg = G2G_ReqLoginToken::internal_default_instance();
	MHandleMap.emplace( hashStr(msg->GetDescriptor()->full_name()), make_pair(msg, &Exe_ReqUserToken));

	msg = COM_RetHeartbeat::internal_default_instance();
	MHandleRetMap.emplace( hashStr(msg->GetDescriptor()->full_name()), make_pair(msg, &Exe_RetHeartbeat));

	msg = C2S_ReqAuthToken::internal_default_instance();
	MHandleMap.emplace( hashStr(msg->GetDescriptor()->full_name()), make_pair(msg, &Msg_ReqAuthToken));

}
