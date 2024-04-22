module;
#include <map>
#include <functional>
#include "hv/Channel.h"

#include "StdAfx.h"
#include "Server/S_Common.pb.h"
export module DatabaseMessage;

export import :DatabaseCommon;

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg::S_Common;

export class DatabaseMessageHandle
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



void DatabaseMessageHandle::MsgHandle(const SocketChannelPtr &channel, uint32_t msgId, size_t msgHashId, const string& msgData)
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

void DatabaseMessageHandle::MsgRetHandle(const SocketChannelPtr &channel, uint32_t msgId, size_t msgHashId, const string &msgData)
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

void DatabaseMessageHandle::RegMsgHandle()
{
	std::hash<string> hashStr;

	MSG_MAPPING(MHandleRetMap, COM_RetChangeCtlSrv, &Exe_RetChangeCtlSrv);
}