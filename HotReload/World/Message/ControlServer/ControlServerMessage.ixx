module;
export module ControlServerMessage;

import :ControlCommon;
import :ControlRedirect;
import MessageRegister;
import StrUtils;

export class ControlServerMessageHandle : public MessageRegister
{

public:

	void RegMsgHandle()
	{
	#define MSG_MAPPING(map, msg, func) \
		map.emplace(DoStringHash(msg::GetDescriptor()->full_name()), \
		make_pair(msg::internal_default_instance(), &ControlServerMessage::func))


		MSG_MAPPING(MHandleMap, GMsg::COM_ReqRegistSrv, Msg_ReqRegistSrv);

		MSG_MAPPING(MHandleRetMap, GMsg::COM_RetHeartbeat, Exe_RetHeartbeat);

		MSG_MAPPING(MHandleRedirectMap, GMsg::A2g_ReqAuthAccount, Msg_ReqAuthAccount);
	}

	std::function<void(const DNServer::Ptr& server)> GetClientRegistFunc()
	{
		return nullptr;
	}

};
