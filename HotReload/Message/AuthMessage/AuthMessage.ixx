module;
#include <functional>
#include <cstdint>
#include "google/protobuf/message.h"
#include "hv/Channel.h"
#include "hv/HttpService.h"

#include "StdMacro.h"
#include "Common/Common.pb.h"
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
	static void MsgRetHandle(SocketChannelPtr channel, size_t msgHashId, const string& msgData);
	static void RegMsgHandle();

	static void RegApiHandle(HttpService* service);
public:
	inline static unordered_map<size_t, pair<const Message*, function<void(SocketChannelPtr, uint32_t, Message*)>>> MHandleMap;
	inline static unordered_map<size_t, pair<const Message*, function<void(SocketChannelPtr, Message*)>>> MHandleRetMap;
};



void AuthMessageHandle::MsgHandle(SocketChannelPtr channel, uint32_t msgId, size_t msgHashId, const string& msgData)
{
	if (MHandleMap.contains(msgHashId))
	{
		auto& handle = MHandleMap[msgHashId];
		Message* message = handle.first->New();
		if (message->ParseFromString(msgData))
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

void AuthMessageHandle::MsgRetHandle(SocketChannelPtr channel, size_t msgHashId, const string& msgData)
{
	if (MHandleRetMap.contains(msgHashId))
	{
		auto& handle = MHandleRetMap[msgHashId];
		Message* message = handle.first->New();
		if (message->ParseFromString(msgData))
		{
			handle.second(channel, message);
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