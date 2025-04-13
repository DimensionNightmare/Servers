module;
export module MessagePack;

import std.compat;
import StrUtils;

export enum class EMMsgDir : uint8_t
{
	Outer = 1, 	// Client Msg
	Inner, 		// Server Msg
};

export enum class EMMsgDeal : uint8_t
{
	Req = 1, 	// msg deal with
	Res, 		// req result
	Ret,		// notify
	Redir,	// Redirect
};

#pragma pack(1) // net struct need this
export struct MessagePacket
{
	static int PackLenth;
	uint32_t pkgLenth = 0;	 //Pin Top !

	EMMsgDir opType = EMMsgDir::Inner;
	EMMsgDeal dealType = EMMsgDeal::Req;
	uint16_t serverId = 0;
	uint32_t msgId = 0;
	size_t msgHashId = 0;
};
#pragma pack()

int MessagePacket::PackLenth = sizeof(MessagePacket);

export bool MessagePack(uint32_t msgId, EMMsgDeal deal, const std::string& pbName, std::string& data)
{
	MessagePacket packet;
	packet.msgId = msgId;
	packet.dealType = deal;
	packet.pkgLenth = uint32_t(data.size());

	if (pbName.empty()) [[unlikely]]
	{
		packet.msgHashId = 0;
	}
	else [[likely]]
	{
		packet.msgHashId = DoStringHash(pbName);
	}

	data.resize(MessagePacket::PackLenth + packet.pkgLenth);

	memmove(data.data() + MessagePacket::PackLenth, data.data(), packet.pkgLenth);
	memcpy(data.data(), &packet, MessagePacket::PackLenth);
	return true;
}
