export module DatabaseServerMessage;

import MessageRegister;
import StrUtils;
import MessagePack;
import std;


class Server;

namespace ServerMessage
{
	export MessageRegister::CVPtr GetMessageHandle()
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
	struct DatabaseTag {};


	export template<typename MsgReq, typename MsgRes, EMMsgDeal msgDeal, typename ServerTag = DatabaseTag>
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
					ServerMessage::GetMessageHandle()->mHandleMap.emplace(msgHash, [this](const auto& a, const auto& b, const auto& c, const auto& d) 
					{ 
						if(!this->bIsCoroutine)
						{
							this->TickMessage(a, b, c, d); 
						}
						else
						{
							this->TickMessageAsync(a, b, c, d); 
						}
					});
					break;
				case EMMsgDeal::Ret:
					ServerMessage::GetMessageHandle()->mHandleRetMap.emplace(msgHash, [this](const auto& a, const auto& b, const auto& c) 
					{ 
						this->TickMessage(a, b, c); 
					});
					break;
				case EMMsgDeal::Redir:
					ServerMessage::GetMessageHandle()->mHandleRedirectMap.emplace(msgHash, [this](const auto& a, const auto& b, const auto& c, const auto& d) 
					{
						if(!this->bIsCoroutine)
						{
							this->TickMessage(a, b, c, d); 
						}
						else
						{
							this->TickMessageAsync(a, b, c, d); 
						}
					});
					break;

			}
		}
	};

	export template<typename ServerTag = DatabaseTag>
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