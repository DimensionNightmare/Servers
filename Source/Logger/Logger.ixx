module;
#include <string>
#include <cstdarg>
#include <format>
#include <iostream>
export module Logger;

import Utils.StrUtils;
import I10nText;

using namespace std;

export enum class LoggerLevel : uint8_t
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

export void LoggerPrint(LoggerLevel level, int code, const char* funcName, const char* fmt,  ...)
{
	if(level < SLogLevel)
	{
		return;
	}

	if(code > 0)
	{
		switch (level)
		{
		case LoggerLevel::Debug:
			
			break;
		case LoggerLevel::Normal:
			fmt = GetTipText(code);
			break;
		
		case LoggerLevel::Error:
			fmt = GetErrText(code);
			break;
		}
	}

	if(!fmt)
	{
		return;
	}

	va_list args;
	va_start(args, fmt);
	size_t len = vsnprintf(0, 0, fmt, args) + 1;
	string message;
	message.resize(len); // need space for NUL
	// va_end(args);

	va_start(args, fmt);
	vsnprintf(&message[0], len, fmt, args);
	va_end(args);

	cout << format("[{}] {} -> \n{}", GetNowTimeStr(), funcName, message) << endl;
}