module;
export module GateMessage;

export import :GateCommon;
import :GateGlobal;
import :GateClient;
import :GateRedirect;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import StrUtils;
import MessageRegister;

export class GateMessageHandle : public MessageRegister
{
public:
	void RegMsgHandle()
	{
	#define MSG_MAPPING(map, msg, func) \
		map.emplace(DoStringHash(msg::GetDescriptor()->full_name()), \
		make_pair(msg::internal_default_instance(), &GateMessage::func))


		MSG_MAPPING(MHandleMap, GMsg::COM_ReqRegistSrv, Msg_ReqRegistSrv);
		MSG_MAPPING(MHandleMap, GMsg::C2S_ReqAuthToken, Msg_ReqAuthToken);
		MSG_MAPPING(MHandleMap, GMsg::A2g_ReqAuthAccount, Exe_ReqUserToken);

		MSG_MAPPING(MHandleRetMap, GMsg::COM_RetHeartbeat, Exe_RetHeartbeat);

		MSG_MAPPING(MHandleRedirectMap, GMsg::L2D_ReqLoadData, Exe_ReqLoadData);
		MSG_MAPPING(MHandleRedirectMap, GMsg::L2D_ReqSaveData, Exe_ReqSaveData);


	}

};
