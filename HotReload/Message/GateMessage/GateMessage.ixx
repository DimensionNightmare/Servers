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

export class GateMessageHandle
{

public:

	static void MsgHandle(const DNSocketProxy::Ptr& channel, uint32_t msgId, size_t msgHashId, const std::string& msgData)
	{
		if (MHandleMap.contains(msgHashId))
		{
			auto& handle = MHandleMap[msgHashId];
			try
			{
				handle.second(channel, msgId, msgData);
			}
			catch (const std::exception& e)
			{
				SPidLogger.Record(ELogLevel_Debug, e.what());
			}
		}
		else
		{
			SPidLogger.Record(EL10nCode_MsgHandleFind);
		}
	}

	static void MsgRetHandle(const DNSocketProxy::Ptr& channel, size_t msgHashId, const std::string& msgData)
	{
		if (MHandleRetMap.contains(msgHashId))
		{
			auto& handle = MHandleRetMap[msgHashId];
			try
			{
				handle.second(channel, msgData);
			}
			catch (const std::exception& e)
			{
				SPidLogger.Record(ELogLevel_Debug, e.what());
			}
		}
		else
		{
			SPidLogger.Record(EL10nCode_MsgHandleFind);
		}
	}

	static void MsgRedirectHandle(const DNSocketProxy::Ptr& channel, uint32_t msgId, size_t msgHashId, const std::string& msgData)
	{
		if (MHandleRedirectMap.contains(msgHashId))
		{
			auto& handle = MHandleRedirectMap[msgHashId];
			try
			{
				handle.second(channel, msgId, msgData);
			}
			catch (const std::exception& e)
			{
				SPidLogger.Record(ELogLevel_Debug, e.what());
			}
		}
		else
		{
			SPidLogger.Record(EL10nCode_MsgHandleFind);
		}
	}

	static void RegMsgHandle()
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

public:
	inline static std::unordered_map<size_t, std::pair<const Message*, std::function<void(DNSocketProxy::Ptr, uint32_t, std::string)>>> MHandleMap;
	inline static std::unordered_map<size_t, std::pair<const Message*, std::function<void(DNSocketProxy::Ptr, std::string)>>> MHandleRetMap;
	inline static std::unordered_map<size_t, std::pair<const Message*, std::function<void(DNSocketProxy::Ptr, uint32_t, std::string)>>> MHandleRedirectMap;
};
