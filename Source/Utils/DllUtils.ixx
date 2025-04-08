module;
#ifdef _WIN32
	#include <libloaderapi.h>
#endif
#include "StdMacro.h"
export module DllUtils;

template<typename T>
struct MemberFunctionReturnType;

template<typename Class, typename ReturnType, typename... Args>
struct MemberFunctionReturnType<ReturnType(Class::*)(Args...)> {
	using RetType = ReturnType;
};

template<typename T>
struct DefaultReturnValue {
	static T get() { return T(); }
};

template<>
struct DefaultReturnValue<void> {
	static void get() {}
};

template <typename Method>
struct MemberFunctionArgs;

template <typename R, typename Class, typename... Args>
struct MemberFunctionArgs<R(Class::*)(Args...)>
{
	using Arguments = std::tuple<Args...>;
};

export template <typename Class, typename Method, typename... Args>
auto TickMainSpaceDll(Class* obj, const char* methodName, Method method, Args... args)
{
	using FuncSignature = decltype(method);
	using ArgsTuple = typename MemberFunctionArgs<FuncSignature>::Arguments;
	using RetType = typename MemberFunctionReturnType<Method>::RetType;
	typedef RetType(*MethodSign)(Class*, ArgsTuple);

	std::string className = typeid(Class).name();
	size_t pos = className.find(" ");
	if (pos != std::string::npos)
	{
		className = className.substr(pos + 1);
	}

	std::string fullFuncName = std::format("{}_{}", className, methodName);

	static std::unordered_map<std::string, void*> cache;

	if (auto it = cache.find(fullFuncName);it != cache.end())
	{
		MethodSign pFuncTyped = reinterpret_cast<MethodSign>(it->second);
		return pFuncTyped(obj, std::make_tuple(std::forward<Args>(args)...));
	}

	if (HMODULE hModule = GetModuleHandle(NULL))
	{
		if (void* pFunc = reinterpret_cast<void*>(GetProcAddress(hModule, fullFuncName.c_str())))
		{
			cache[fullFuncName] = pFunc;
			MethodSign pFuncTyped = reinterpret_cast<MethodSign>(pFunc);
			return pFuncTyped(obj, std::make_tuple(std::forward<Args>(args)...));
		}
	}

	return DefaultReturnValue<RetType>::get();
}
