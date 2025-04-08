module;
#include <cstdarg>

#include "StdMacro.h"
export module Logger;

import StrUtils;
import L10nText;
import ThirdParty.PbGen;

ELogLevel SLogLevel = ELogLevel_Normal;

class LogColor {
public:
    inline static std::string RED	= "\033[31m";
    inline static std::string GREEN	= "\033[32m";
    inline static std::string YELLOW	= "\033[33m";
    inline static std::string BLUE	= "\033[34m";
    inline static std::string RESET	= "\033[0m";
};

std::ofstream LogFile; 

/// @brief set logger type and Log file Init 
export void SetLoggerLevel(std::optional<ELogLevel> level = std::nullopt, std::optional<std::filesystem::path> path = std::nullopt)
{
	if(level)
	{
		SLogLevel = *level;
	}
	
	if(!LogFile.is_open() && path)
	{
		if(!std::filesystem::exists(*path))
		{
			std::filesystem::create_directories(*path);
		}
		LogFile = std::ofstream( std::format("{}/Output.log", path.value().string()), std::ios::app);
	}
}

/// @brief Log file pointer 
std::ofstream* GetLoggerFile()
{
	if (!LogFile.is_open())
	{
		return nullptr;
	}

	return &LogFile;
}

/// @brief print char to command-line/log-file. 
export void LoggerPrint(EL10nCode code, const char* funcName, ...)
{
	ELogLevel level = ELogLevel_Debug;
	const char* fmt = nullptr;

	DNl10n::GetTipText(code, level, fmt);

	if (level < SLogLevel || !fmt)
	{
		return;
	}

	va_list args;
	va_start(args, funcName);
	size_t len = vsnprintf(0, 0, fmt, args);
	static std::string message;
	message.resize(len); // need space for NUL
	// va_end(args);

	va_start(args, funcName);
	vsnprintf(&message[0], len + 1, fmt, args);
	va_end(args);

	std::string outputStr = std::format("[{}] {} -> \n\t{}\n", GetNowTimeStr(), funcName, message);
	switch (level)
	{
		case ELogLevel_Normal:
			std::cout << LogColor::BLUE << outputStr << LogColor::RESET;
			break;
		case ELogLevel_Warning:
			std::cout << LogColor::YELLOW << outputStr << LogColor::RESET;
			break;
		case ELogLevel_Error:
			std::cout << LogColor::RED << outputStr << LogColor::RESET;
			break;
		default:
			std::cout << outputStr;
			return;
	}

	if(std::ofstream* file = GetLoggerFile())
	{
		*file << outputStr;
		file->flush();
	}
}

/// @brief print char to command-line/log-file. 
export void LoggerPrint(ELogLevel level, const char* fmt, const char* funcName, ...)
{
	if (level < SLogLevel || !fmt)
	{
		return;
	}

	va_list args;
	va_start(args, funcName);
	size_t len = vsnprintf(0, 0, fmt, args);
	static std::string message;
	message.resize(len); // need space for NUL
	// va_end(args);

	va_start(args, funcName);
	vsnprintf(&message[0], len + 1, fmt, args);
	va_end(args);

	std::string outputStr = std::format("[{}] {} -> \n\t{}\n", GetNowTimeStr(), funcName, message);
	switch (level)
	{
		case ELogLevel_Normal:
			std::cout << LogColor::BLUE << outputStr << LogColor::RESET;
			break;
		case ELogLevel_Warning:
			std::cout << LogColor::YELLOW << outputStr << LogColor::RESET;
			break;
		case ELogLevel_Error:
			std::cout << LogColor::RED << outputStr << LogColor::RESET;
			break;
		default:
			std::cout << outputStr;
			return;
	}

	if(std::ofstream* file = GetLoggerFile())
	{
		*file << outputStr;
		file->flush();
	}
}
