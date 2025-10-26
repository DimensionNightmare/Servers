export module AuthServerMessage;

import MessageRegister;
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
	struct AuthTag{};

	export template<typename ServerTag = AuthTag>
	class HandleClientRegistry
	{
	public:
		template<typename Executor>
		HandleClientRegistry(Executor&& executor)
		{
			ServerMessage::GetMessageHandle()->pClientRegistFunc = std::forward<Executor>(executor);
		}
	};

	export template<typename ServerTag = AuthTag>
	class HandleApiRegistry
	{
	public:
		template<typename Executor>
		HandleApiRegistry(Executor&& executor)
		{
			ServerMessage::GetMessageHandle()->pApiRegistFunc = std::forward<Executor>(executor);
		}
	};
}