module;
#include <string>
export module MessagePack;

using namespace std;

// define
enum class MsgDir : unsigned char
{
    Outer, //Client Msg
    Inner, //Server Msg
};

export struct MessagePacket
{
    unsigned int pkgLenth;
    MsgDir opType;
	unsigned short serverId;
    unsigned int msgId;

    MessagePacket()
    {
        memset(this, 0 , sizeof *this);
    }
};

// export bool MessagePack(Message* reqMsg, MessagePacket& packet);
// export bool MessageUnpack(char* reqMsg, int len);

// implement
module:private;

// bool MessagePack(Message* reqMsg, MessagePacket& packet)
// {
//     if(reqMsg == nullptr)
//         return false;

//     // mp.opType = MsgDir::Outer;
//     // mp.msgId = msgId;
    
//     string name;
    
//     return true;
// }

// bool MessageUnpack(char* reqMsg, int len)
// {
//     return true;
// }