module;
#include "Common.pb.h"

#include <string>
#include <format>
#include <cstdarg>
export module Logger;

import Utils.StrUtils;
import I10nText;

using namespace std;

export enum class LoggerLevel
{
	Debug,
	Normal,
	Waring,
	Error,
};

LoggerLevel SLogLevel = LoggerLevel::Normal;

export void SetLoggerLevel(LoggerLevel level)
{
	SLogLevel = level;
}

export void LoggerPrint(LoggerLevel level, size_t code, const char* funcName, const char* fmt,  ...)
{
	if(level < SLogLevel)
	{
		return;
	}

	va_list args;
	va_start(args, fmt);

	if(!fmt)
	{
		switch (level)
		{
		case LoggerLevel::Debug:
		case LoggerLevel::Normal:
			fmt = GetTipText((TipCode)code);
			break;
		
		case LoggerLevel::Error:
			fmt = GetErrText((ErrCode)code);
			break;
		}
	}

	if(!fmt)
	{
		return;
	}
	
	size_t len = vsnprintf(0, 0, fmt, args);
	string message;
	message.resize(len + 1);  // need space for NUL
    vsnprintf(&message[0], len + 1,fmt, args);
    message.resize(len);
	va_end(args);

	std::cout << format("[{}] {} -> \n{}", GetNowTimeStr(), funcName, message) << std::endl;
}