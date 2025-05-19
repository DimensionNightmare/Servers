module;
export module ControlMessage;

import :ControlCommon;
import :ControlRedirect;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import StrUtils;
import MessageRegister;

export class ControlMessageHandle : public MessageRegister
{

public:

	void RegMsgHandle()
	{
	#define MSG_MAPPING(map, msg, func) \
		map.emplace(DoStringHash(msg::GetDescriptor()->full_name()), \
		make_pair(msg::internal_default_instance(), &ControlMessage::func))


		MSG_MAPPING(MHandleMap, GMsg::COM_ReqRegistSrv, Msg_ReqRegistSrv);

		MSG_MAPPING(MHandleRetMap, GMsg::COM_RetHeartbeat, Exe_RetHeartbeat);

		MSG_MAPPING(MHandleRedirectMap, GMsg::A2g_ReqAuthAccount, Msg_ReqAuthAccount);
	}
};
