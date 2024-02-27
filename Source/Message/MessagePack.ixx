module;

#include <string>
export module MessagePack;

using namespace std;

export enum class MsgDir : unsigned char
{
	Outer = 1, 	// Client Msg
	Inner, 		// Server Msg
};

export enum class MsgDeal : unsigned char
{
	Req = 1, 	// msg deal with
	Res, 		//
	Ret,
};

#pragma pack(1) // net struct need this
export struct MessagePacket
{
	static int PackLenth; 
	unsigned int pkgLenth;	 //Pin Top !

	MsgDir opType;
	MsgDeal dealType;
	unsigned short serverId;
	unsigned int msgId;
	size_t msgHashId;

	MessagePacket()
	{
		memset(this, 0, sizeof *this);
	}
};

#pragma pack()

int MessagePacket::PackLenth = sizeof MessagePacket;

export bool MessagePack(unsigned int msgId, MsgDeal deal, const char* pbName, string &data);

// implement
module :private;

bool MessagePack(unsigned int msgId, MsgDeal deal,  const char* pbName, string &data)
{
	MessagePacket packet;
	packet.msgId = msgId;
	packet.dealType = deal;
	packet.pkgLenth = unsigned int(data.size());

	if( pbName == nullptr) [[unlikely]]
	{	
		packet.msgHashId = 0;
	}
	else [[likely]]
	{
		packet.msgHashId = std::hash<string>::_Do_hash(pbName);
	}

	data.resize(MessagePacket::PackLenth + packet.pkgLenth);
	
	memmove_s(data.data() + MessagePacket::PackLenth, packet.pkgLenth, data.data(), packet.pkgLenth);
	memcpy(data.data(), &packet, MessagePacket::PackLenth);
	return true;
}