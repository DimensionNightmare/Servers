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
		make_pair(msg::internal_default_instance(), &GlobalServerMessage::func))

		MSG_MAPPING(MHandleMap, GMsg::COM_ReqRegistSrv, Msg_ReqRegistSrv);

		MSG_MAPPING(MHandleRetMap, GMsg::g2G_RetRegistSrv, Exe_RetRegistSrv);
		MSG_MAPPING(MHandleRetMap, GMsg::COM_RetHeartbeat, Exe_RetHeartbeat);
		MSG_MAPPING(MHandleRetMap, GMsg::g2G_RetRegistChild, Exe_RetRegistChild);

		MSG_MAPPING(MHandleRedirectMap, GMsg::A2g_ReqAuthAccount, Msg_ReqAuthAccount);
	}
	
	std::function<void(const DNServer::Ptr& server)> GetClientRegistFunc()
	{
		return &GlobalServerMessage::Evt_ReqRegistSrv;
	}

};
