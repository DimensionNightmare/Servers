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
	#pragma comment(lib, "dbghelp.lib")
#elif __unix__
	#include <dlfcn.h>
	#include <csignal>
	#include <execinfo.h>
	#include <fcntl.h>
	#include <sys/stat.h>
#endif
export module Platform;

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

export
{
	using ::HMODULE;
	using ::GetModuleHandleA;
	using ::GetProcAddress;
	using ::SetConsoleTitleA;
	using ::LoadLibraryA;
	using ::FreeLibrary;
	using ::GetLastError;
	using ::GetPrivateProfileSectionNamesA;
	using ::GetPrivateProfileSectionA;
	using ::CreateProcessA;
	using ::PROCESS_INFORMATION;
	using ::STARTUPINFOA;
	using ::_EXCEPTION_POINTERS;
	using ::_MINIDUMP_EXCEPTION_INFORMATION;
	using ::CreateFileA;
	using ::SetUnhandledExceptionFilter;
	using ::GetCurrentThreadId;
	using ::GetCurrentProcess;
	using ::GetCurrentProcessId;
	using ::MINIDUMP_TYPE;
	using ::CloseHandle;
	using ::Sleep;
	using ::MiniDumpWriteDump;
	using ::SetConsoleCtrlHandler;
	using ::SetEnvironmentVariableA;
}

#endif
