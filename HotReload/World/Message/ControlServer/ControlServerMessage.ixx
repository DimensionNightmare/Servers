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
			make_pair(msg::internal_default_instance(), func))


		MSG_MAPPING(MHandleMap, GMsg::COM_ReqRegistSrv, &ControlServerMessage::Msg_ReqRegistSrv);

		MSG_MAPPING(MHandleRetMap, GMsg::COM_RetHeartbeat, &ControlServerMessage::Exe_RetHeartbeat);

		MSG_MAPPING(MHandleRedirectMap, GMsg::A2g_ReqAuthAccount, &ControlServerMessage::Msg_ReqAuthAccount);

	#undef MSG_MAPPING
	}

	std::function<void(Server::CVPtr)> GetClientRegistFunc()
	{
		return nullptr;
	}

};
