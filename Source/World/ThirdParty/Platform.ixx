module;
#if _WIN32
	#include <windows.h>
	#include <dbghelp.h>
	#include <crtdbg.h>
	#pragma comment(lib, "dbghelp.lib")
	#include <conio.h>
#elif __unix__
	#include <dlfcn.h>
	#include <csignal>
	#include <execinfo.h>
	#include <fcntl.h>
	#include <sys/stat.h>
#endif
export module ThirdParty.Platform;

import std.compat;

#ifdef _WIN32

export namespace Platform
{
	using HotHandle = ::HMODULE;	
	using FuncHandle = ::FARPROC;
	using ::LoadLibraryA;
	using ::FreeLibrary;
	using ::GetProcAddress;
	using ::Sleep;

	using ::SetConsoleTitleA;
	using ::GetLastError;
	using ::GetPrivateProfileSectionNamesA;
	using ::GetPrivateProfileSectionA;
	using ::CreateProcessA;
	using ::PROCESS_INFORMATION;
	using ::STARTUPINFOA;
	using ::_EXCEPTION_POINTERS;
	using ::_MINIDUMP_EXCEPTION_INFORMATION;
	using ::CreateFileA;
	using ::CloseHandle;
	using ::SetUnhandledExceptionFilter;
	using ::GetCurrentThreadId;
	using ::GetCurrentProcess;
	using ::GetCurrentProcessId;
	using ::MINIDUMP_TYPE;
	using ::MiniDumpWriteDump;
	using ::SetConsoleCtrlHandler;
	using ::SetEnvironmentVariableA;

	using ::sockaddr_in;
	using ::sockaddr;
	using ::ntohs;
	using ::getsockname;

	using ::_kbhit;
	using ::_getch;

	// using InvalidHandle = INVALID_HANDLE_VALUE;
}

export namespace Platform
{
	std::string GetStackTrace(unsigned int begin = 0)
	{
		HANDLE process = GetCurrentProcess();
		SymInitialize(process, NULL, TRUE);

		void* stack[128];
		unsigned short frames = CaptureStackBackTrace(1, 128, stack, NULL);

		std::ostringstream oss;
		SYMBOL_INFO* symbol = static_cast<SYMBOL_INFO*>(malloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char)));
		symbol->MaxNameLen = 255;
		symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

		IMAGEHLP_LINE64 line;
		line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
		DWORD displacement;

		for (unsigned int i = begin; i < frames; i++)
		{
			DWORD64 address = reinterpret_cast<DWORD64>(stack[i]);
			SymFromAddr(process, address, 0, symbol);
			if (SymGetLineFromAddr64(process, address, &displacement, &line))
			{
				oss << "#" << i << " " << symbol->Name << " ("
					<< line.FileName << ":" << line.LineNumber << ")\n";
			}
			else
			{
				oss << "#" << i << " " << symbol->Name << " (0x" << stack[i] << ")\n";
			}
		}

		free(symbol);
		SymCleanup(process);
		return oss.str();
	}

	void SetDebugFlag()
	{
		// _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
		// _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
		// _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
		// _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG);

	}

	std::string GetExecutablePath()
	{
#if _WIN32
		char path[MAX_PATH];
		GetModuleFileNameA(NULL, path, MAX_PATH); // 获取完整路径
		return std::string(path);
#endif
	}

	/// @brief get runtime lib funcpointer
	FuncHandle GetFuncPtr(HotHandle handle, const char* funcName)
	{
#ifdef _WIN32
		return GetProcAddress(handle, funcName);
#elif __unix__
		return dlsym(handle, funcName);
#endif
		return nullptr;
	}
}

#endif
