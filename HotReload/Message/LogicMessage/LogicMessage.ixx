module;
export module LogicMessage;

export import :LogicCommon;
import :LogicGate;
import :LogicRedirect;
import :LogicDedicated;
import Logger;
import ThirdParty.Libhv;
import ThirdParty.PbGen;
import StrUtils;

export class LogicMessageHandle
{

public:

	static void MsgHandle(const SocketChannelPtr& channel, uint32_t msgId, size_t msgHashId, const std::string& msgData)
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
				LoggerPrint()(ELogLevel_Debug, e.what());
			}
		}
		else
		{
			LoggerPrint()(EL10nCode_MsgHandleFind);
		}
	}

	static void MsgRetHandle(const SocketChannelPtr& channel, size_t msgHashId, const std::string& msgData)
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
				LoggerPrint()(ELogLevel_Debug, e.what());
			}
		}
		else
		{
			LoggerPrint()(EL10nCode_MsgHandleFind);
		}
	}

	static void MsgRedirectHandle(const SocketChannelPtr& channel, uint32_t msgId, size_t msgHashId, const std::string& msgData)
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
				LoggerPrint()(ELogLevel_Debug, e.what());
			}
		}
		else
		{
			LoggerPrint()(EL10nCode_MsgHandleFind);
		}
	}

	static void RegMsgHandle()
	{
	#define MSG_MAPPING(map, msg, func) \
		map.emplace(DoStringHash(msg::GetDescriptor()->full_name()), \
		make_pair(msg::internal_default_instance(), &LogicMessage::func))


		MSG_MAPPING(MHandleMap, d2L_ReqRegistSrv, Msg_ReqRegistSrv);
		MSG_MAPPING(MHandleMap, d2L_ReqLoadEntityData, Msg_ReqLoadEntityData);

		MSG_MAPPING(MHandleRetMap, COM_RetChangeCtlSrv, Exe_RetChangeCtlSrv);
		MSG_MAPPING(MHandleRetMap, COM_RetHeartbeat, Exe_RetHeartbeat);
		MSG_MAPPING(MHandleRetMap, g2L_RetProxyOffline, Exe_RetProxyOffline);
		MSG_MAPPING(MHandleRetMap, d2L_ReqSaveEntityData, Msg_ReqSaveEntityData);


		MSG_MAPPING(MHandleRedirectMap, S2C_RetAccountReplace, Exe_RetAccountReplace);
		MSG_MAPPING(MHandleRedirectMap, C2S_ReqAuthToken, Msg_ReqClientLogin);
	}
public:

	inline static std::unordered_map<size_t, std::pair<const Message*, std::function<void(SocketChannelPtr, uint32_t, std::string)>>> MHandleMap;

	inline static std::unordered_map<size_t, std::pair<const Message*, std::function<void(SocketChannelPtr, std::string)>>> MHandleRetMap;

	inline static std::unordered_map<size_t, std::pair<const Message*, std::function<void(SocketChannelPtr, uint32_t, std::string)>>> MHandleRedirectMap;
};
