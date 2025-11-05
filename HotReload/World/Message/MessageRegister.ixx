export module MessageRegister;

import Logger;
import ThirdParty.Libhv;
import Server;
import std;
import ThirdParty.PbGen;
import MessagePack;
import FuncHelper;
import Task;

// HandleRegistry<GMsg::C2S_ReqAuthToken, GMsg::S2C_ResAuthToken, EMMsgDeal::Req> Msg_ReqAuthToken =
// 				[](auto request, auto response, SocketChannel::Ptr channel) -> TaskVoid
// 	{
// 	};

export struct MessageRegister
{

public:

	void MsgHandle(SocketChannel::Ptr channel, uint32_t msgId, size_t msgHashId, const std::string& msgData)
	{
		if (mHandleMap.contains(msgHashId))
		{
			auto& handle = mHandleMap[msgHashId];
			try
			{
				handle(channel, msgId, msgData);
			}
			catch (const std::exception& e)
			{
				LoggerPrint::Log(channel, ELogLevel_Debug, "{}", e.what());
			}
		}
		else
		{
			LoggerPrint::Log(channel, EL10nCode_MsgHandleFind);
		}
	}

	void MsgRetHandle(SocketChannel::Ptr channel, size_t msgHashId, const std::string& msgData)
	{
		if (mHandleRetMap.contains(msgHashId))
		{
			auto& handle = mHandleRetMap[msgHashId];
			try
			{
				handle(channel, msgData);
			}
			catch (const std::exception& e)
			{
				LoggerPrint::Log(channel, ELogLevel_Debug, "{}", e.what());
			}
		}
		else
		{
			LoggerPrint::Log(channel, EL10nCode_MsgHandleFind);
		}
	}

	void MsgRedirectHandle(SocketChannel::Ptr channel, uint32_t msgId, size_t msgHashId, const std::string& msgData)
	{
		if (mHandleRedirectMap.contains(msgHashId))
		{
			auto& handle = mHandleRedirectMap[msgHashId];
			try
			{
				handle(channel, msgId, msgData);
			}
			catch (const std::exception& e)
			{
				LoggerPrint::Log(channel, ELogLevel_Debug, "{}", e.what());
			}
		}
		else
		{
			LoggerPrint::Log(channel, EL10nCode_MsgHandleFind);
		}
	}

public:
	std::unordered_map<size_t, std::function<void(SocketChannel::Ptr, uint32_t, const std::string&)>> mHandleMap;
	std::unordered_map<size_t, std::function<void(SocketChannel::Ptr, const std::string&)>> mHandleRetMap;
	std::unordered_map<size_t, std::function<void(SocketChannel::Ptr, uint32_t, const std::string&)>> mHandleRedirectMap;

	std::function<void(Server::Ptr)> pClientRegistFunc;
	std::function<void(Server::Ptr)> pApiRegistFunc;
};

export template<typename MsgReq, typename MsgRes, EMMsgDeal msgDeal, typename ServerTag>
class MessageRegistry
{
public:

	template<typename Executor>
	requires std::is_void_v<MsgRes> && std::invocable<Executor, MsgReq*, SocketChannel::Ptr>
	MessageRegistry(Executor&& executor)
	{
		oInvoker = [executor = std::forward<Executor>(executor)](MessageRegistry* self, SocketChannel::Ptr channel)
			{
				executor(&self->oMsgReq, channel);
			};
	}

	template<typename Executor>
	requires (!std::is_void_v<MsgRes>) 
		&& std::invocable<Executor, MsgReq*, MsgRes*, SocketChannel::Ptr>
		&& std::is_void_v<std::invoke_result_t<Executor, MsgReq*, MsgRes*, SocketChannel::Ptr>>
	MessageRegistry(Executor&& executor)
	{
		oInvoker = [executor = std::forward<Executor>(executor)](MessageRegistry* self, SocketChannel::Ptr channel)
			{
				executor(&self->oMsgReq, &self->oMsgRes, channel);
			};
	}

	template<typename Executor>
	requires (!std::is_void_v<MsgRes>) 
		&& std::invocable<Executor, MsgReq*, MsgRes*, SocketChannel::Ptr, std::function<void()>>
		&& std::is_void_v<std::invoke_result_t<Executor, MsgReq*, MsgRes*, SocketChannel::Ptr, std::function<void()>>>
	MessageRegistry(Executor&& executor)
	{
		oInvoker = [executor = std::forward<Executor>(executor)](MessageRegistry* self, SocketChannel::Ptr channel)
			{
				executor(&self->oMsgReq, &self->oMsgRes, channel, [=](){self->Reply(channel);});
			};
	}

	template<typename Executor>
	requires (!std::is_void_v<MsgRes>) 
		&& std::invocable<Executor, MsgReq*, MsgRes*, SocketChannel::Ptr>
		&& std::is_same_v<std::invoke_result_t<Executor, MsgReq*, MsgRes*, SocketChannel::Ptr>, TaskVoid>
	MessageRegistry(Executor&& executor)
	{
		bIsCoroutine = true;

		oAsyncInvoker = [executor = std::forward<Executor>(executor)](MessageRegistry* self, SocketChannel::Ptr channel) ->TaskVoid
			{
				co_await executor(&self->oMsgReq, &self->oMsgRes, channel);
				co_return;
			};
	}

	virtual ~MessageRegistry()
	{
		
	}

protected:

	MessageRegistry(const MessageRegistry& other)
		: oInvoker(other.oInvoker), oAsyncInvoker(other.oAsyncInvoker)
	{
	}

	virtual void RegistMsg(){}

	void Reply(SocketChannel::Ptr channel)
	{
		if(bIsReplyed)
		{
			return;
		}
		
		bIsReplyed = true;

		if constexpr (!std::is_void_v<MsgRes>)
		{
			MessagePackAndSend(iMsgId, EMMsgDeal::Res, &oMsgRes, channel);
		}
	}

	bool ParseRequestMsg(const std::string& binMsg)
	{
		return oMsgReq.ParseFromString(binMsg);
	}

	void TickMessage(SocketChannel::Ptr channel, const std::string& binMsg)
	{
		MessageRegistry exector = *this;

		if (!exector.ParseRequestMsg(binMsg))
		{
			return;
		}

		exector.oInvoker(&exector, channel);
	}

	void TickMessage(SocketChannel::Ptr channel, uint32_t msgId, const std::string& binMsg)
	{
		MessageRegistry exector = *this;

		if (!exector.ParseRequestMsg(binMsg))
		{
			return;
		}

		exector.iMsgId = msgId;

		exector.oInvoker(&exector, channel);

		exector.Reply(channel);
	}

	TaskVoid TickMessageAsync(SocketChannel::Ptr channel, uint32_t msgId, const std::string& binMsg)
	{
		MessageRegistry exector = *this;

		if (!exector.ParseRequestMsg(binMsg))
		{
			co_return;
		}

		exector.iMsgId = msgId;

		co_await exector.oAsyncInvoker(&exector, channel);

		exector.Reply(channel);

		co_return;
	}



protected:

	uint32_t iMsgId = 0;
	bool bIsReplyed = false;

	MsgReq oMsgReq;
	std::conditional_t<std::is_void_v<MsgRes>, std::monostate, MsgRes> oMsgRes;
	std::function<void(MessageRegistry*, SocketChannel::Ptr)> oInvoker;
	std::function<TaskVoid(MessageRegistry*, SocketChannel::Ptr)> oAsyncInvoker;

	bool bIsCoroutine = false;
};
