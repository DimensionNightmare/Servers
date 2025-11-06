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

	void MsgHandle(const SocketChannel::Ptr& channel, uint32_t msgId, size_t msgHashId, const std::string& msgData)
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

	void MsgRetHandle(const SocketChannel::Ptr& channel, size_t msgHashId, const std::string& msgData)
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

	void MsgRedirectHandle(const SocketChannel::Ptr& channel, uint32_t msgId, size_t msgHashId, const std::string& msgData)
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
	std::unordered_map<size_t, std::function<void(const SocketChannel::Ptr&, uint32_t, const std::string&)>> mHandleMap;
	std::unordered_map<size_t, std::function<void(const SocketChannel::Ptr&, const std::string&)>> mHandleRetMap;
	std::unordered_map<size_t, std::function<void(const SocketChannel::Ptr&, uint32_t, const std::string&)>> mHandleRedirectMap;

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
		oInvoker = [executor = std::forward<Executor>(executor)](MessageRegistry* self, const SocketChannel::Ptr& channel)
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
		oInvoker = [executor = std::forward<Executor>(executor)](MessageRegistry* self, const SocketChannel::Ptr& channel)
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
		oInvoker = [executor = std::forward<Executor>(executor)](MessageRegistry* self, const SocketChannel::Ptr& channel)
			{
				executor(&self->oMsgReq, &self->oMsgRes, channel, self->pReplyProxy);
			};
	}

	template<typename Executor>
	requires (!std::is_void_v<MsgRes>) 
		&& std::invocable<Executor, MsgReq*, MsgRes*, SocketChannel::Ptr>
		&& std::is_same_v<std::invoke_result_t<Executor, MsgReq*, MsgRes*, SocketChannel::Ptr>, TaskVoid>
	MessageRegistry(Executor&& executor)
	{
		bIsCoroutine = true;

		oAsyncInvoker = [executor = std::forward<Executor>(executor)](MessageRegistry* self, const SocketChannel::Ptr& channel) ->TaskVoid
			{
				co_await executor(&self->oMsgReq, &self->oMsgRes, channel);
				co_return;
			};
	}

protected:

	MessageRegistry()
	{
	}

	MessageRegistry(uint32_t msgId, const SocketChannel::Ptr& channel)
	{
		pReplyProxy = [=]() mutable
		{
			Reply(msgId, channel);
		};
	}

	virtual ~MessageRegistry()
	{
		pReplyProxy
			? pReplyProxy()
			: void();
	}

	MessageRegistry(const MessageRegistry& other)
		: oInvoker(other.oInvoker), oAsyncInvoker(other.oAsyncInvoker)
	{
	}

	virtual void RegistMsg(){}

	void Reply(uint32_t msgId, const SocketChannel::Ptr& channel)
	{
		if(bIsReplyed)
		{
			return;
		}
		
		bIsReplyed = true;

		if constexpr (!std::is_void_v<MsgRes>)
		{
			MessagePackAndSend(msgId, EMMsgDeal::Res, &oMsgRes, channel);
		}
	}

	bool ParseRequestMsg(const std::string& binMsg)
	{
		return oMsgReq.ParseFromString(binMsg);
	}

	void TickMessage(const SocketChannel::Ptr& channel, const std::string& binMsg)
	{
		MessageRegistry exector;

		if (!exector.ParseRequestMsg(binMsg))
		{
			return;
		}

		oInvoker(&exector, channel);
	}

	void TickMessage(const SocketChannel::Ptr& channel, uint32_t msgId, const std::string& binMsg)
	{
		MessageRegistry exector(msgId, channel);

		if (!exector.ParseRequestMsg(binMsg))
		{
			return;
		}

		oInvoker(&exector, channel);
	}

	TaskVoid TickMessageAsync(const SocketChannel::Ptr& channel, uint32_t msgId, const std::string& binMsg)
	{
		MessageRegistry exector(msgId, channel);

		if (!exector.ParseRequestMsg(binMsg))
		{
			co_return;
		}

		co_await oAsyncInvoker(&exector, channel);

		co_return;
	}



protected:

	// 副本
	bool bIsReplyed = false;
	MsgReq oMsgReq;
	std::conditional_t<std::is_void_v<MsgRes>, std::monostate, MsgRes> oMsgRes;
	std::function<void()> pReplyProxy;
	
	// 唯一执行器 副本不应该持有
	std::function<void(MessageRegistry*, const SocketChannel::Ptr&)> oInvoker;
	std::function<TaskVoid(MessageRegistry*, const SocketChannel::Ptr&)> oAsyncInvoker;
	bool bIsCoroutine = false;
};
