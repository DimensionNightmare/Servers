module;
export module GateServerMessage;

import :GateCommon;
import :GateGlobal;
import :GateClient;
import :GateRedirect;
import MessageRegister;

export class GateServerMessageHandle : public MessageRegister
{
public:
	void RegMsgHandle()
	{
		#define MSG_MAPPING(map, msg, func) \
			map.emplace(DoStringHash(msg::GetDescriptor()->full_name()), \
			make_pair(msg::internal_default_instance(), func))


		MSG_MAPPING(MHandleMap, GMsg::COM_ReqRegistSrv, &GateServerMessage::Msg_ReqRegistSrv);
		MSG_MAPPING(MHandleMap, GMsg::C2S_ReqAuthToken, &GateServerMessage::Msg_ReqAuthToken);
		MSG_MAPPING(MHandleMap, GMsg::A2g_ReqAuthAccount, &GateServerMessage::Exe_ReqUserToken);

		MSG_MAPPING(MHandleRetMap, GMsg::COM_RetHeartbeat, &GateServerMessage::Exe_RetHeartbeat);

		MSG_MAPPING(MHandleRedirectMap, GMsg::L2D_ReqLoadData, &GateServerMessage::Exe_ReqLoadData);
		MSG_MAPPING(MHandleRedirectMap, GMsg::L2D_ReqSaveData, &GateServerMessage::Exe_ReqSaveData);

		#undef MSG_MAPPING
	}

	std::function<void(Server::CVPtr)> GetClientRegistFunc()
	{
		return &GateServerMessage::Evt_ReqRegistSrv;
	}

};
