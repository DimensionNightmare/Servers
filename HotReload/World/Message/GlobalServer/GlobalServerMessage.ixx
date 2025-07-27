module;
export module GlobalServerMessage;

import :GlobalCommon;
import :GlobalControl;
import :GlobalGate;
import :GlobalRedirect;
import MessageRegister;
import StrUtils;

export class GlobalServerMessageHandle : public MessageRegister
{

public:

	void RegMsgHandle()
	{
		#define MSG_MAPPING(map, msg, func) \
			map.emplace(DoStringHash(msg::GetDescriptor()->full_name()), \
			make_pair(msg::internal_default_instance(), func))

		MSG_MAPPING(MHandleMap, GMsg::COM_ReqRegistSrv, &GlobalServerMessage::Msg_ReqRegistSrv);

		MSG_MAPPING(MHandleRetMap, GMsg::g2G_RetRegistSrv, &GlobalServerMessage::Exe_RetRegistSrv);
		MSG_MAPPING(MHandleRetMap, GMsg::COM_RetHeartbeat, &GlobalServerMessage::Exe_RetHeartbeat);
		MSG_MAPPING(MHandleRetMap, GMsg::g2G_RetRegistChild, &GlobalServerMessage::Exe_RetRegistChild);

		MSG_MAPPING(MHandleRedirectMap, GMsg::A2g_ReqAuthAccount, &GlobalServerMessage::Msg_ReqAuthAccount);

		#undef MSG_MAPPING
	}
	
	std::function<void(Server::CVPtr)> GetClientRegistFunc()
	{
		return &GlobalServerMessage::Evt_ReqRegistSrv;
	}

};
