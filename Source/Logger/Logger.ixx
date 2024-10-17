module;
#include "StdMacro.h"
#include <cstdarg>
export module Logger;

import StrUtils;
import I10nText;

export enum class LoggerLevel : uint8_t
{
	Debug,
	Normal,
	Warning,
	Error,
};

LoggerLevel SLogLevel = LoggerLevel::Normal;

ofstream LogFile; 

/// @brief set logger type and Log file Init 
export void SetLoggerLevel(optional<LoggerLevel> level = nullopt, optional<string_view> path = nullopt)
{
	if(level.has_value())
	{
		SLogLevel = level.value();
	}
	
	if(!LogFile.is_open() && path.has_value())
	{
		LogFile = ofstream( format("{}/Output.log", path.value()), std::ios::app);
	}
}

/// @brief Log file pointer 
ofstream* GetLoggerFile()
{
	if (!LogFile.is_open())
	{
		return nullptr;
	}

	return &LogFile;
}

/// @brief print char to command-line/log-file. 
export void LoggerPrint(LoggerLevel level, int code, const char* funcName, const char* fmt, ...)
{
	if (level < SLogLevel)
	{
		return;
	}

	if (code)
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

	if (!fmt)
	{
		return;
	}

	va_list args;
	va_start(args, fmt);
	size_t len = vsnprintf(0, 0, fmt, args);
	static string message;
	message.resize(len); // need space for NUL
	// va_end(args);

	va_start(args, fmt);
	vsnprintf(&message[0], len + 1, fmt, args);
	va_end(args);

	string outputStr = format("[{}] {} -> \n{}\n", GetNowTimeStr(), funcName, message);
	cout << outputStr;

	if(ofstream* file = GetLoggerFile())
	{
		*file << outputStr;
		file->flush();
	}
}
