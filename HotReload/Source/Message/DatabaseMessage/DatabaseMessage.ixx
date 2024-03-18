module;
#include "StdAfx.h"
#include "S_Common.pb.h"
#include "hv/Channel.h"

#include <map>
#include <functional>
export module DatabaseMessage;

export import :DatabaseCommon;


using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg::S_Common;

export class DatabaseMessageHandle
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



void DatabaseMessageHandle::MsgHandle(const SocketChannelPtr &channel, unsigned int msgId, size_t msgHashId, const string& msgData)
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

void DatabaseMessageHandle::MsgRetHandle(const SocketChannelPtr &channel, unsigned int msgId, size_t msgHashId, const string &msgData)
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

void DatabaseMessageHandle::RegMsgHandle()
{
	std::hash<string> hashStr;
	const Message* msg = nullptr;

	msg = COM_RetChangeCtlSrv::internal_default_instance();
	MHandleRetMap.emplace( hashStr(msg->GetDescriptor()->full_name()), make_pair(msg, &Exe_RetChangeCtlSrv));
}