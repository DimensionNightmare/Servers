module;
#include "google/protobuf/message.h"
#include "AuthControl.pb.h"
#include "hv/Channel.h"
#include "hv/HttpService.h"

#include <map>
#include <functional>
export module AuthMessage;

export import :AuthCommon;
import ApiManager;
import AfxCommon;

#define DNPrint(code, level, ...) LoggerPrint(level, code, __FUNCTION__, ##__VA_ARGS__);

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg::AuthControl;

export class AuthMessageHandle
{
public:
	static void MsgHandle(const SocketChannelPtr &channel, unsigned int msgId, size_t msgHashId, const string& msgData);
	static void RegMsgHandle();

	static void RegApiHandle(HttpService* service);
public:
	inline static map<
		size_t, 
		pair<
			const Message*, 
			function<void(const SocketChannelPtr &, unsigned int, Message *)> 
		> 
	> MHandleMap;
};

module :private;

void AuthMessageHandle::MsgHandle(const SocketChannelPtr &channel, unsigned int msgId, size_t msgHashId, const string& msgData)
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

void AuthMessageHandle::RegMsgHandle()
{

}

void AuthMessageHandle::RegApiHandle(HttpService* service)
{
	ApiInit(service);
}