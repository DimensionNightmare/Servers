module;
#include <map>
#include <functional>
#include <cstdint>
#include "google/protobuf/message.h"
#include "hv/Channel.h"
#include "hv/HttpService.h"

#include "StdAfx.h"
export module AuthMessage;

export import :AuthCommon;
import ApiManager;


using namespace std;
using namespace hv;
using namespace google::protobuf;

export class AuthMessageHandle
{
public:
	static void MsgHandle(SocketChannelPtr channel, uint32_t msgId, size_t msgHashId, const string& msgData);
	static void MsgRetHandle(SocketChannelPtr channel, uint32_t msgId, size_t msgHashId, const string& msgData);
	static void RegMsgHandle();

	static void RegApiHandle(HttpService* service);
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



void AuthMessageHandle::MsgHandle(SocketChannelPtr channel, uint32_t msgId, size_t msgHashId, const string& msgData)
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

void AuthMessageHandle::MsgRetHandle(SocketChannelPtr channel, uint32_t msgId, size_t msgHashId, const string &msgData)
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

void AuthMessageHandle::RegMsgHandle()
{

}

void AuthMessageHandle::RegApiHandle(HttpService* service)
{
	service->Static("/", "./");
	
	ApiInit(service);
}