export module FuncHelper;

import MessagePack;
import ThirdParty.Libhv;
import StrUtils;
import Logger;
import std.compat;
import ThirdParty.Protobuf;

static constexpr size_t BYTES_PER_LINE = 16;

void printHexDump(const std::string& data, const std::string& title = "Hex Dump")
{
	std::cout << "\n" << title << " (" << data.length() << " bytes)\n";
	std::cout << std::string(78, '=') << "\n";

	for (size_t offset = 0; offset < data.length(); offset += BYTES_PER_LINE)
	{
		// 地址偏移
		std::cout << std::format("{:08x}  ", offset);

		// 十六进制数据
		std::string hexPart;
		std::string asciiPart;

		for (size_t i = 0; i < BYTES_PER_LINE; ++i)
		{
			if (offset + i < data.length())
			{
				unsigned char byte = data[offset + i];
				hexPart += std::format("{:02x} ", byte);

				// ASCII 表示（可打印字符显示，不可打印显示点）
				asciiPart += (byte >= 32 && byte < 127) ? static_cast<char>(byte) : '.';
			}
			else
			{
				hexPart += "   ";
				asciiPart += " ";
			}

			// 8字节分隔符
			if (i == 7) hexPart += " ";
		}

		std::cout << hexPart << " |" << asciiPart << "|\n";
	}
	std::cout << std::string(78, '=') << "\n";
}

export 
{

	void MessagePackAndSend(uint32_t msgId, EMMsgDeal deal, Message* message, SocketChannel::CVPtr channel)
	{
		size_t hash = 0;
		std::string msgData;
		std::string msgName;

		if(deal != EMMsgDeal::Res)
		{
			msgName = message->GetDescriptor()->full_name();
			hash = DoStringHash(msgName);
		}

		message->SerializeToString(&msgData);

		MessagePack(msgId, deal, hash, msgData);
		channel->write(msgData);

		LoggerPrint::Log(channel, ELogLevel_Debug, "{} Send type={} With Mid:{}, Mess:{}", channel->peeraddr(), EnumName(deal), msgId, msgName);
		// printHexDump(msgData);
	}

}
