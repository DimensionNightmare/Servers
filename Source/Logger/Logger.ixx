module;
#include <string>
#include <cstdarg>
#include <format>
#include <iostream>
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
	size_t len = vsnprintf(0, 0, fmt, args);
	string message;
	message.resize(len + 1);  // need space for NUL
    vsnprintf(&message[0], len + 1,fmt, args);
    message.resize(len);
	va_end(args);

	cout << format("[{}] {} -> \n{}", GetNowTimeStr(), funcName, message) << endl;
}