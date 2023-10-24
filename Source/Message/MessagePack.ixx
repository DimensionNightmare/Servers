module;
#include <string>
export module MessagePack;

using namespace std;

export enum class MsgDir : unsigned char
{
	Outer = 1, // Client Msg
	Inner, // Server Msg
};

#pragma pack(1) // net struct need this
export struct MessagePacket
{
	static int PackLenth; 
	unsigned int pkgLenth;	 //Pin Top !
	MsgDir opType;
	unsigned char serverId;
	unsigned char msgId;
	unsigned char msgLenth;

	MessagePacket()
	{
		memset(this, 0, sizeof *this);
	}
};

#pragma pack()

int MessagePacket::PackLenth = sizeof MessagePacket;

export bool MessagePack(unsigned int msgId, MsgDir dir, const string &pbName, string &data);

// implement
module :private;

bool MessagePack(unsigned int msgId, MsgDir dir,  const string &pbName, string &data)
{
	int dataLen = (int)data.size();
	int msgNameLen = (int)pbName.size();

	MessagePacket packet;
	packet.msgId = msgId;
	packet.opType = dir;
	packet.msgLenth = msgNameLen;
	packet.pkgLenth = dataLen + msgNameLen;
	data.resize(MessagePacket::PackLenth + packet.pkgLenth);
	
	memmove_s(data.data() + MessagePacket::PackLenth + msgNameLen, dataLen, data.data(), dataLen);
	memcpy(data.data() + MessagePacket::PackLenth, pbName.data(), msgNameLen);
	memcpy(data.data(), &packet, MessagePacket::PackLenth);
	return true;
}