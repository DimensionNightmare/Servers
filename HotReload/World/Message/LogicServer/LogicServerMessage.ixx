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
			make_pair(msg::internal_default_instance(), func))


		MSG_MAPPING(MHandleMap, GMsg::d2L_ReqRegistSrv, &LogicServerMessage::Msg_ReqRegistSrv);
		MSG_MAPPING(MHandleMap, GMsg::d2L_ReqLoadEntityData, &LogicServerMessage::Msg_ReqLoadEntityData);

		MSG_MAPPING(MHandleRetMap, GMsg::COM_RetChangeCtlSrv, &LogicServerMessage::Exe_RetChangeCtlSrv);
		MSG_MAPPING(MHandleRetMap, GMsg::COM_RetHeartbeat, &LogicServerMessage::Exe_RetHeartbeat);
		MSG_MAPPING(MHandleRetMap, GMsg::g2L_RetProxyOffline, &LogicServerMessage::Exe_RetProxyOffline);
		MSG_MAPPING(MHandleRetMap, GMsg::d2L_ReqSaveEntityData, &LogicServerMessage::Msg_ReqSaveEntityData);


		MSG_MAPPING(MHandleRedirectMap, GMsg::S2C_RetAccountReplace, &LogicServerMessage::Exe_RetAccountReplace);
		MSG_MAPPING(MHandleRedirectMap, GMsg::C2S_ReqAuthToken, &LogicServerMessage::Msg_ReqClientLogin);

		#undef MSG_MAPPING
	}
	
	std::function<void(Server::CVPtr)> GetClientRegistFunc()
	{
		return &LogicServerMessage::Evt_ReqRegistSrv;
	}

};
