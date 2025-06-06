module;
export module LogicServerMessage;

import :LogicCommon;
import :LogicGate;
import :LogicRedirect;
import :LogicDedicated;
import MessageRegister;
import StrUtils;

export class LogicServerMessageHandle : public MessageRegister
{

public:

	void RegMsgHandle()
	{
	#define MSG_MAPPING(map, msg, func) \
		map.emplace(DoStringHash(msg::GetDescriptor()->full_name()), \
		make_pair(msg::internal_default_instance(), &LogicServerMessage::func))


		MSG_MAPPING(MHandleMap, GMsg::d2L_ReqRegistSrv, Msg_ReqRegistSrv);
		MSG_MAPPING(MHandleMap, GMsg::d2L_ReqLoadEntityData, Msg_ReqLoadEntityData);

		MSG_MAPPING(MHandleRetMap, GMsg::COM_RetChangeCtlSrv, Exe_RetChangeCtlSrv);
		MSG_MAPPING(MHandleRetMap, GMsg::COM_RetHeartbeat, Exe_RetHeartbeat);
		MSG_MAPPING(MHandleRetMap, GMsg::g2L_RetProxyOffline, Exe_RetProxyOffline);
		MSG_MAPPING(MHandleRetMap, GMsg::d2L_ReqSaveEntityData, Msg_ReqSaveEntityData);


		MSG_MAPPING(MHandleRedirectMap, GMsg::S2C_RetAccountReplace, Exe_RetAccountReplace);
		MSG_MAPPING(MHandleRedirectMap, GMsg::C2S_ReqAuthToken, Msg_ReqClientLogin);
	}
	
	std::function<void(const DNServer::Ptr& server)> GetClientRegistFunc()
	{
		return &LogicServerMessage::Evt_ReqRegistSrv;
	}

};
