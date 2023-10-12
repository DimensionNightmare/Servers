module;
#include <string>
export module MessagePack;

using namespace std;

// declare
class Message;
enum class MsgDir : unsigned char;
struct MessagePacket;

export bool MessagePack(Message* reqMsg, MessagePacket& packet);
export bool MessageUnpack(char* reqMsg, int len);

// define
enum class MsgDir : unsigned char
{
    Outer, //Client Msg
    Inner, //Server Msg
};

struct MessagePacket
{
    unsigned int pkgLenth;
    MsgDir opType;
	unsigned short serverId;
    unsigned int msgId;

    Message* reqData; //virtual
    Message* retData; //virtual
    MessagePacket()
    {
        memset(this, 0 , sizeof *this);
    }
};

// implement
module:private;

bool MessagePack(Message* reqMsg, MessagePacket& packet)
{
    if(reqMsg == nullptr)
        return false;

    // mp.opType = MsgDir::Outer;
    // mp.msgId = msgId;
    
    string name;
    
    return true;
}

bool MessageUnpack(char* reqMsg, int len)
{
    return true;
}