module;
#include <concepts>

#if _WIN32
	// #include <libloaderapi.h>
	// #include <windef.h>
	// #include <consoleapi.h>
	// #include <consoleapi2.h>
	// #include <WinBase.h>
	// #include <windef.h>
	// #include <verrsrc.h>
	// #include <fileapi.h>
	// #include <timezoneapi.h>
	// #include <errhandlingapi.h>
	// #include <processthreadsapi.h>
	// #include <minidumpapiset.h>
	// #include <synchapi.h>
	// #include <handleapi.h>
	#include <windows.h>
	#include <dbghelp.h>
	#include <crtdbg.h>
	#pragma comment(lib, "dbghelp.lib")
#elif __unix__
	#include <dlfcn.h>
	#include <csignal>
	#include <execinfo.h>
	#include <fcntl.h>
	#include <sys/stat.h>
#endif
export module ThirdParty.Platform;

import std.compat;

template <typename F>
concept NoArgCallable = requires(F f) {
    { std::invoke(f) } -> std::same_as<void>;
};

template <NoArgCallable F>
auto make_wrapper(F&& f) {
    return [f=std::forward<F>(f)]() { 
        f(); 
    };
}

template <typename F>
auto make_wrapper(F&& f) requires (!NoArgCallable<F>) {
    return [f=std::forward<F>(f)](auto&&... args) -> decltype(auto) {
        return f(std::forward<decltype(args)>(args)...);
    };
}

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
}

export namespace Platform
{
	std::string GetStackTrace()
	{
		HANDLE process = GetCurrentProcess();
		SymInitialize(process, NULL, TRUE);

		void* stack[128];
		unsigned short frames = CaptureStackBackTrace(1, 128, stack, NULL);

		std::ostringstream oss;
		SYMBOL_INFO* symbol = (SYMBOL_INFO*)malloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char));
		symbol->MaxNameLen = 255;
		symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

		IMAGEHLP_LINE64 line;
		line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
		DWORD displacement;

		for (unsigned int i = 0; i < frames; i++)
		{
			DWORD64 address = (DWORD64)(stack[i]);
			SymFromAddr(process, address, 0, symbol);
			if (SymGetLineFromAddr64(process, address, &displacement, &line))
			{
				oss << "#" << i << " " << symbol->Name << " ("
					<< line.FileName << ":" << line.LineNumber << ")\n";
			}
			else
			{
				oss << "#" << i << " " << symbol->Name << " (0x" << (void*)address << ")\n";
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
}

#endif
