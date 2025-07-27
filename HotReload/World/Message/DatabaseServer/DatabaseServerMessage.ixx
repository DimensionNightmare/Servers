module;
export module DatabaseServerMessage;

import :DatabaseCommon;
import :DatabaseGate;

import MessageRegister;
import StrUtils;

export class DatabaseServerMessageHandle : public MessageRegister
{

public:

	void RegMsgHandle()
	{
		#define MSG_MAPPING(map, msg, func) \
			map.emplace(DoStringHash(msg::GetDescriptor()->full_name()), \
			make_pair(msg::internal_default_instance(), func))


		MSG_MAPPING(MHandleMap, GMsg::L2D_ReqLoadData, &DatabaseServerMessage::Exe_ReqLoadData);
		MSG_MAPPING(MHandleMap, GMsg::L2D_ReqSaveData, &DatabaseServerMessage::Exe_ReqSaveData);

		MSG_MAPPING(MHandleRetMap, GMsg::COM_RetChangeCtlSrv, &DatabaseServerMessage::Exe_RetChangeCtlSrv);

		#undef MSG_MAPPING
	}

	std::function<void(Server::CVPtr)> GetClientRegistFunc()
	{
		return &DatabaseServerMessage::Evt_ReqRegistSrv;
	}

};
