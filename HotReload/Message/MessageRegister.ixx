module;

export module MessageRegister;

import DNSocketProxy;
import std.compat;
import Logger;
import ThirdParty.PbGen;
import ECSW;

export class MessageRegister
{

public:

	void MsgHandle(const World::Ptr& world, const DNSocketProxy::Ptr& channel, uint32_t msgId, size_t msgHashId, const std::string& msgData)
	{
		if (MHandleMap.contains(msgHashId))
		{
			auto& handle = MHandleMap[msgHashId];
			try
			{
				handle.second(world, channel, msgId, msgData);
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

	void MsgRetHandle(const World::Ptr& world, const DNSocketProxy::Ptr& channel, size_t msgHashId, const std::string& msgData)
	{
		if (MHandleRetMap.contains(msgHashId))
		{
			auto& handle = MHandleRetMap[msgHashId];
			try
			{
				handle.second(world, channel, msgData);
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

	void MsgRedirectHandle(const World::Ptr& world, const DNSocketProxy::Ptr& channel, uint32_t msgId, size_t msgHashId, const std::string& msgData)
	{
		if (MHandleRedirectMap.contains(msgHashId))
		{
			auto& handle = MHandleRedirectMap[msgHashId];
			try
			{
				handle.second(world, channel, msgId, msgData);
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

	virtual void RegMsgHandle() = 0;

protected:
	std::unordered_map<size_t, std::pair<const Message*, std::function<void(const World::Ptr&, const DNSocketProxy::Ptr&, uint32_t, std::string)>>> MHandleMap;
	std::unordered_map<size_t, std::pair<const Message*, std::function<void(const World::Ptr&, const DNSocketProxy::Ptr&, std::string)>>> MHandleRetMap;
	std::unordered_map<size_t, std::pair<const Message*, std::function<void(const World::Ptr&, const DNSocketProxy::Ptr&, uint32_t, std::string)>>> MHandleRedirectMap;
};