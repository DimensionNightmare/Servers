module;
#include <map>
#include <functional>
#include "hv/Channel.h"

#include "StdAfx.h"
#include "Server/S_Common.pb.h"
#include "Server/S_Logic.pb.h"
export module LogicMessage;

export import :LogicCommon;
export import :LogicGate;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg::S_Common;
using namespace GMsg::S_Logic;

export class LogicMessageHandle
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



void LogicMessageHandle::MsgHandle(const SocketChannelPtr &channel, unsigned int msgId, size_t msgHashId, const string& msgData)
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

void LogicMessageHandle::MsgRetHandle(const SocketChannelPtr &channel, unsigned int msgId, size_t msgHashId, const string &msgData)
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

void LogicMessageHandle::RegMsgHandle()
{
	std::hash<string> hashStr;
	const Message* msg = nullptr;

	msg = COM_RetChangeCtlSrv::internal_default_instance();
	MHandleRetMap.emplace( hashStr(msg->GetDescriptor()->full_name()), make_pair(msg, &Exe_RetChangeCtlSrv));

	msg = COM_ReqRegistSrv::internal_default_instance();
	MHandleMap.emplace( hashStr(msg->GetDescriptor()->full_name()), make_pair(msg, &Msg_ReqRegistSrv));

	msg = COM_RetHeartbeat::internal_default_instance();
	MHandleRetMap.emplace( hashStr(msg->GetDescriptor()->full_name()), make_pair(msg, &Exe_RetHeartbeat));

	msg = G2L_ReqClientLogin::internal_default_instance();
	MHandleMap.emplace( hashStr(msg->GetDescriptor()->full_name()), make_pair(msg, &Msg_ReqClientLogin));
}