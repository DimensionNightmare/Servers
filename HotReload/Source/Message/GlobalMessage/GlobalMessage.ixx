module;
#include "StdAfx.h"
#include "GlobalControl.pb.h"
#include "AuthGlobal.pb.h"
#include "CommonMsg.pb.h"
#include "GateGlobal.pb.h"
#include "hv/Channel.h"

#include <map>
#include <functional>
export module GlobalMessage;

export import :GlobalControl;
import :GlobalCommon;
import :GlobalAuth;
import :GlobalGate;


using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg::CommonMsg;
using namespace GMsg::AuthGlobal;
using namespace GMsg::GateGlobal;

export class GlobalMessageHandle
{
public:
	static void MsgHandle(const SocketChannelPtr &channel, unsigned int msgId, size_t msgHashId, const string& msgData);
	static void RegMsgHandle();
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

void GlobalMessageHandle::MsgHandle(const SocketChannelPtr &channel, unsigned int msgId, size_t msgHashId, const string& msgData)
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

void GlobalMessageHandle::RegMsgHandle()
{
	std::hash<string> hashStr;
	const Message* msg = nullptr;

	msg = COM_ReqRegistSrv::internal_default_instance();
	MHandleMap.emplace( hashStr(msg->GetDescriptor()->full_name()), make_pair(msg, &Exe_RegistSrv));

	msg = A2G_AuthAccount::internal_default_instance();
	MHandleMap.emplace( hashStr(msg->GetDescriptor()->full_name()), make_pair(msg, &Exe_AuthAccount));

	msg = G2G_RetRegistSrv::internal_default_instance();
	MHandleMap.emplace( hashStr(msg->GetDescriptor()->full_name()), make_pair(msg, &Exe_RetRegistSrv));
}