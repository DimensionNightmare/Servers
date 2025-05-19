module;
export module DatabaseMessage;

export import :DatabaseCommon;
import :DatabaseGate;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import StrUtils;
import MessageRegister;

export class DatabaseMessageHandle : public MessageRegister
{

public:

	void RegMsgHandle()
	{
	#define MSG_MAPPING(map, msg, func) \
		map.emplace(DoStringHash(msg::GetDescriptor()->full_name()), \
		make_pair(msg::internal_default_instance(), &DatabaseMessage::func))


		MSG_MAPPING(MHandleMap, GMsg::L2D_ReqLoadData, Exe_ReqLoadData);
		MSG_MAPPING(MHandleMap, GMsg::L2D_ReqSaveData, Exe_ReqSaveData);

		MSG_MAPPING(MHandleRetMap, GMsg::COM_RetChangeCtlSrv, Exe_RetChangeCtlSrv);
	}
};
