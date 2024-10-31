module;
#include <cstdarg>

#include "StdMacro.h"
export module Logger;

import StrUtils;
import I10nText;

export enum class EMLoggerLevel : uint8_t
{
	Debug,
	Normal,
	Warning,
	Error,
};

EMLoggerLevel SLogLevel = EMLoggerLevel::Normal;

ofstream LogFile; 

/// @brief set logger type and Log file Init 
export void SetLoggerLevel(optional<EMLoggerLevel> level = nullopt, optional<string_view> path = nullopt)
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
export void LoggerPrint(EMLoggerLevel level, int code, const char* funcName, const char* fmt, ...)
{
	if (level < SLogLevel)
	{
		return;
	}

	if (code)
	{
		switch (level)
		{
			case EMLoggerLevel::Debug:

				break;
			case EMLoggerLevel::Normal:
				fmt = GetTipText(code);
				break;

			case EMLoggerLevel::Error:
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

	string outputStr = format("[{}] {} -> \n\t{}\n", GetNowTimeStr(), funcName, message);
	cout << outputStr;

	if(ofstream* file = GetLoggerFile())
	{
		*file << outputStr;
		file->flush();
	}
}
