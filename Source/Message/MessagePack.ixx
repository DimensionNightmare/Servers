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
	unsigned char serverId;
	unsigned char msgId;
	unsigned int msgLenth;

	MessagePacket()
	{
		memset(this, 0, sizeof *this);
	}
};

#pragma pack()

int MessagePacket::PackLenth = sizeof MessagePacket;

export bool MessagePack(unsigned int msgId, MsgDeal deal, const string &pbName, string &data);

// implement
module :private;

bool MessagePack(unsigned int msgId, MsgDeal deal,  const string &pbName, string &data)
{
	int dataLen = (int)data.size();
	int msgNameLen = (int)pbName.size();

	MessagePacket packet;
	packet.msgId = msgId;
	packet.dealType = deal;
	packet.msgLenth = msgNameLen;
	packet.pkgLenth = dataLen + msgNameLen;
	data.resize(MessagePacket::PackLenth + packet.pkgLenth);
	
	memmove_s(data.data() + MessagePacket::PackLenth + msgNameLen, dataLen, data.data(), dataLen);
	memcpy(data.data() + MessagePacket::PackLenth, pbName.data(), msgNameLen);
	memcpy(data.data(), &packet, MessagePacket::PackLenth);
	return true;
}