module;
#include "GlobalControl.pb.h"
#include "AuthControl.pb.h"
#include "Common.pb.h"
#include "GateGlobal.pb.h"
#include "hv/Channel.h"

#include <map>
#include <functional>
export module GlobalMessage;

export import :GlobalControl;
import AfxCommon;
import :GlobalCommon;
import :GlobalAuth;
import :GlobalGate;

#define DNPrint(fmt, ...) printf("[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr().c_str(), __FUNCTION__, ##__VA_ARGS__);
#define DNPrintErr(fmt, ...) fprintf(stderr, "[%s] {%s} ->" "\n" fmt "\n", GetNowTimeStr().c_str(), __FUNCTION__, ##__VA_ARGS__);

using namespace std;
using namespace hv;
using namespace google::protobuf;
using namespace GMsg::Common;
using namespace GMsg::AuthControl;
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
		auto message = handle.first->New();
		if(message->ParseFromArray(msgData.data(), msgData.length()))
		{	
			handle.second(channel, msgId, message);
		}
		else
		{
			DNPrintErr("cant parse msg Deal Handle! \n");
		}
		
		delete message;
	}
	else
	{
		DNPrintErr("cant find msgid Deal Handle! \n");
	}
}

void GlobalMessageHandle::RegMsgHandle()
{
	std::hash<string> hashStr;
	const Message* msg = nullptr;

	msg = COM_ReqRegistSrv::internal_default_instance();
	MHandleMap.emplace( hashStr(msg->GetDescriptor()->full_name()), make_pair(msg, &Exe_RegistSrv));

	msg = A2C_AuthAccount::internal_default_instance();
	MHandleMap.emplace( hashStr(msg->GetDescriptor()->full_name()), make_pair(msg, &Exe_AuthAccount));

	msg = G2G_RetRegistSrv::internal_default_instance();
	MHandleMap.emplace( hashStr(msg->GetDescriptor()->full_name()), make_pair(msg, &Exe_RetRegistSrv));
}