module;
#include <string>
export module MessagePack;

using namespace std;

// define
enum class MsgDir : unsigned char
{
	Outer, // Client Msg
	Inner, // Server Msg
};

export struct MessagePacket
{
	static int HeadLen;
	unsigned int pkgLenth;
	MsgDir opType;
	unsigned short serverId;
	unsigned int msgId;

	MessagePacket()
	{
		memset(this, 0, sizeof *this);
	}
};

int MessagePacket::HeadLen = sizeof MessagePacket;

export bool MessagePack(unsigned int msgId, string &data);
// export bool MessageUnpack(char* reqMsg, int len);

// implement
module :private;

bool MessagePack(unsigned int msgId, string &data)
{
	MessagePacket packet;
	packet.msgId = msgId;
	int datalen = data.size();
	packet.pkgLenth = datalen;
	data.resize(MessagePacket::HeadLen + datalen);
	
	memmove_s(data.data() + MessagePacket::HeadLen, datalen, data.data(), datalen);
	memcpy(data.data(), &packet, MessagePacket::HeadLen);
	return true;
}

// bool MessageUnpack(char* reqMsg, int len)
// {
//     return true;
// }