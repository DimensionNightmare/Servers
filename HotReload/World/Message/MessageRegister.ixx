export module MessageRegister;

import Logger;
import ThirdParty.Libhv;
import Server;
import std;
import ThirdParty.PbGen;
import MessagePack;
import FuncHelper;
import Task;
import ECSW;

export struct MessageRegister
{

public:

	using CVPtr = const std::shared_ptr<MessageRegister>&;

	void MsgHandle(World::CVPtr world, SocketChannel::CVPtr channel, uint32_t msgId, size_t msgHashId, const std::string& msgData)
	{
		if (mHandleMap.contains(msgHashId))
		{
			const auto& handle = mHandleMap[msgHashId];
			try
			{
				handle(world, channel, msgId, msgData);
			}
			catch (const std::exception& e)
			{
				LoggerPrint::Log(world, ELogLevel_Debug, "{}", e.what());
			}
		}
		else
		{
			LoggerPrint::Log(world, EL10nCode_MsgHandleFind);
		}
	}

	void MsgRetHandle(World::CVPtr world, SocketChannel::CVPtr channel, size_t msgHashId, const std::string& msgData)
	{
		if (mHandleRetMap.contains(msgHashId))
		{
			const auto& handle = mHandleRetMap[msgHashId];
			try
			{
				handle(world, channel, msgData);
			}
			catch (const std::exception& e)
			{
				LoggerPrint::Log(world, ELogLevel_Debug, "{}", e.what());
			}
		}
		else
		{
			LoggerPrint::Log(world, EL10nCode_MsgHandleFind);
		}
	}

	void MsgRedirectHandle(World::CVPtr world, SocketChannel::CVPtr channel, uint32_t msgId, size_t msgHashId, const std::string& msgData)
	{
		if (mHandleRedirectMap.contains(msgHashId))
		{
			const auto& handle = mHandleRedirectMap[msgHashId];
			try
			{
				handle(world, channel, msgId, msgData);
			}
			catch (const std::exception& e)
			{
				LoggerPrint::Log(world, ELogLevel_Debug, "{}", e.what());
			}
		}
		else
		{
			LoggerPrint::Log(world, EL10nCode_MsgHandleFind);
		}
	}

public:
	std::unordered_map<size_t, std::function<void(World::CVPtr, SocketChannel::CVPtr, uint32_t, const std::string&)>> mHandleMap;
	std::unordered_map<size_t, std::function<void(World::CVPtr, SocketChannel::CVPtr, const std::string&)>> mHandleRetMap;
	std::unordered_map<size_t, std::function<void(World::CVPtr, SocketChannel::CVPtr, uint32_t, const std::string&)>> mHandleRedirectMap;

	std::function<void(Server::CVPtr)> pClientRegistFunc;
	std::function<void(Server::CVPtr)> pApiRegistFunc;
};

export template<typename MsgReq, typename MsgRes, EMMsgDeal msgDeal, typename ServerTag>
class MessageRegistry
{
public:

	template<typename Executor>
	requires std::is_void_v<MsgRes> && std::invocable<Executor, World::CVPtr, SocketChannel::CVPtr, MsgReq*>
	MessageRegistry(Executor&& executor)
	{
		oInvoker = [executor = std::forward<Executor>(executor)](World::CVPtr world, SocketChannel::CVPtr channel, MessageRegistry* self)
			{
				executor(world, channel, &self->oMsgReq);
			};
	}

	template<typename Executor>
	requires (!std::is_void_v<MsgRes>) 
		&& std::invocable<Executor, World::CVPtr, SocketChannel::CVPtr, MsgReq*, MsgRes*>
		&& std::is_void_v<std::invoke_result_t<Executor, World::CVPtr, SocketChannel::CVPtr, MsgReq*, MsgRes*>>
	MessageRegistry(Executor&& executor)
	{
		oInvoker = [executor = std::forward<Executor>(executor)](World::CVPtr world, SocketChannel::CVPtr channel, MessageRegistry* self)
			{
				executor(world, channel, &self->oMsgReq, &self->oMsgRes);
			};
	}

	template<typename Executor>
	requires (!std::is_void_v<MsgRes>) 
		&& std::invocable<Executor, World::CVPtr, SocketChannel::CVPtr, MsgReq*, MsgRes*, std::function<void()>>
		&& std::is_void_v<std::invoke_result_t<Executor, World::CVPtr, SocketChannel::CVPtr, MsgReq*, MsgRes*, std::function<void()>>>
	MessageRegistry(Executor&& executor)
	{
		oInvoker = [executor = std::forward<Executor>(executor)](World::CVPtr world, SocketChannel::CVPtr channel, MessageRegistry* self)
			{
				executor(world, channel, &self->oMsgReq, &self->oMsgRes, self->pReplyProxy);
			};
	}

	template<typename Executor>
	requires (!std::is_void_v<MsgRes>) 
		&& std::invocable<Executor, World::CVPtr, SocketChannel::CVPtr, MsgReq*, MsgRes*>
		&& std::is_same_v<std::invoke_result_t<Executor, World::CVPtr, SocketChannel::CVPtr, MsgReq*, MsgRes*>, TaskVoid>
	MessageRegistry(Executor&& executor)
	{
		bIsCoroutine = true;

		oAsyncInvoker = [executor = std::forward<Executor>(executor)](World::CVPtr world, SocketChannel::CVPtr channel, MessageRegistry* self) -> TaskVoid
			{
				co_await executor(world, channel, &self->oMsgReq, &self->oMsgRes);
				co_return;
			};
	}

protected:

	MessageRegistry()
	{
	}

	MessageRegistry(uint32_t msgId, SocketChannel::CVPtr channel, World::CVPtr world)
	{
		pReplyProxy = [=]() mutable
		{
			Reply(msgId, channel, world);
		};
	}

	virtual ~MessageRegistry()
	{
		pReplyProxy
			? pReplyProxy()
			: void();
	}

	virtual void RegistMsg(){}

	void Reply(uint32_t msgId, SocketChannel::CVPtr channel, World::CVPtr world)
	{
		if(bIsReplyed)
		{
			return;
		}
		
		bIsReplyed = true;

		if constexpr (!std::is_void_v<MsgRes>)
		{
			MessagePackAndSend(msgId, EMMsgDeal::Res, &oMsgRes, channel, world);
		}
	}

	bool ParseRequestMsg(const std::string& binMsg)
	{
		return oMsgReq.ParseFromString(binMsg);
	}

	void TickMessage(World::CVPtr world, SocketChannel::CVPtr channel, const std::string& binMsg)
	{
		MessageRegistry exector;

		if (!exector.ParseRequestMsg(binMsg))
		{
			return;
		}

		oInvoker(world, channel, &exector);
	}

	void TickMessage(World::CVPtr world, SocketChannel::CVPtr channel, uint32_t msgId, const std::string& binMsg)
	{
		MessageRegistry exector(msgId, channel, world);

		if (!exector.ParseRequestMsg(binMsg))
		{
			return;
		}

		oInvoker(world, channel, &exector);
	}

	TaskVoid TickMessageAsync(World::CVPtr world, SocketChannel::CVPtr channel, uint32_t msgId, const std::string& binMsg)
	{
		MessageRegistry exector(msgId, channel, world);

		if (!exector.ParseRequestMsg(binMsg))
		{
			co_return;
		}

		co_await oAsyncInvoker(world, channel, &exector);

		co_return;
	}



protected:

	// 副本
	bool bIsReplyed = false;
	MsgReq oMsgReq;
	std::conditional_t<std::is_void_v<MsgRes>, std::monostate, MsgRes> oMsgRes;
	std::function<void()> pReplyProxy;
	
	// 唯一执行器 副本不应该持有
	std::function<void(World::CVPtr, SocketChannel::CVPtr, MessageRegistry*)> oInvoker;
	std::function<TaskVoid(World::CVPtr, SocketChannel::CVPtr, MessageRegistry*)> oAsyncInvoker;
	bool bIsCoroutine = false;
};
