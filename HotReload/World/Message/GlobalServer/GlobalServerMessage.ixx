export module GlobalServerMessage;

import MessageRegister;
import StrUtils;
import MessagePack;
import std;

class Server;

namespace ServerMessage
{
	export std::shared_ptr<MessageRegister> GetMessageHandle()
	{
		static std::shared_ptr<MessageRegister> PInstance;
		if (PInstance == nullptr)
		{
			PInstance = std::make_shared<MessageRegister>();
		}

		return PInstance;
	}
}

namespace MsgHandleRegister
{
	struct GlobalTag {};

	export template<typename MsgReq, typename MsgRes, EMMsgDeal msgDeal, typename ServerTag = GlobalTag>
		class HandleRegistry : public MessageRegistry<MsgReq, MsgRes, msgDeal, ServerTag>
	{
	public:

		using BaseType = MessageRegistry<MsgReq, MsgRes, msgDeal, ServerTag>;

		template<typename Executor>
		HandleRegistry(Executor&& executor)
			: BaseType(std::forward<Executor>(executor))
		{
			RegistMsg();
		}

		void RegistMsg()
		{
			size_t msgHash = DoStringHash(MsgReq::GetDescriptor()->full_name());
			switch (msgDeal)
			{
				case EMMsgDeal::Req:
					ServerMessage::GetMessageHandle()->mHandleMap.emplace(msgHash, [this](auto a, auto b, auto c) 
					{ 
						if(!this->bIsCoroutine)
						{
							this->TickMessage(a, b, c); 
						}
						else
						{
							this->TickMessageAsync(a, b, c); 
						}
					});
					break;
				case EMMsgDeal::Ret:
					ServerMessage::GetMessageHandle()->mHandleRetMap.emplace(msgHash, [this](auto a, auto b) 
					{ 
						this->TickMessage(a, b); 
					});
					break;
				case EMMsgDeal::Redir:
					ServerMessage::GetMessageHandle()->mHandleRedirectMap.emplace(msgHash, [this](auto a, auto b, auto c) 
					{
						if(!this->bIsCoroutine)
						{
							this->TickMessage(a, b, c); 
						}
						else
						{
							this->TickMessageAsync(a, b, c); 
						}
					});
					break;

			}
		}
	};

	export template<typename ServerTag = GlobalTag>
	class HandleClientRegistry
	{
	public:
		template<typename Executor>
		HandleClientRegistry(Executor&& executor)
		{
			ServerMessage::GetMessageHandle()->pClientRegistFunc = std::forward<Executor>(executor);
		}
	};

}
