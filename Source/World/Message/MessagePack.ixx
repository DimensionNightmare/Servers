export module MessagePack;

import std.compat;

export
{
	
	enum class EMMsgDir : uint8_t
	{
		None = 0,
		Outer = 1, 	// Client Msg
		Inner, 		// Server Msg
	};
	
	enum class EMMsgDeal : uint8_t
	{
		None = 0,
		Req = 1, 	// msg deal with
		Res, 		// req result
		Ret,		// notify
		Redir,	// Redirect
	};
	
	#pragma pack(1) // net struct need this
	struct MessagePacket
	{
		static int PackLenth;
	
		MessagePacket()
		{
			
		}
	
		static MessagePacket* From(void* data)
		{
			return reinterpret_cast<MessagePacket*>(data);
		}
	
		const char* MsgBegin()
		{
			static_assert(std::is_pointer_v<decltype(this)>,
						"This method must be called through a pointer");
	
			return reinterpret_cast<const char*>(this) + PackLenth;
		}
	
		uint32_t pkgLenth = 0;	 //Pin Top !
	
		EMMsgDir opType = EMMsgDir::Inner;
		EMMsgDeal dealType = EMMsgDeal::Req;
		uint16_t serverId = 0;
		uint32_t msgId = 0;
		size_t msgHashId = 0;
	};
	#pragma pack()
	
	int MessagePacket::PackLenth = sizeof(MessagePacket);
	
	bool MessagePack(uint32_t msgId, EMMsgDeal deal, size_t hashId, std::string& data)
	{
		MessagePacket packet;
		packet.msgId = msgId;
		packet.dealType = deal;
		packet.pkgLenth = static_cast<uint32_t>(data.size());
		packet.msgHashId = hashId;
		
		data.insert(0, reinterpret_cast<const char*>(&packet), MessagePacket::PackLenth);
		return true;
	}

}

