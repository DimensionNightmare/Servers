module;
#include <tuple>
#include <string>
#ifdef _WIN32
	#include <libloaderapi.h>
#endif
#include <unordered_map>
export module Macro;

using namespace std;

template <typename Method>
struct MemberFunctionArgs;

template <typename R, typename Class, typename... Args>
struct MemberFunctionArgs<R(Class::*)(Args...)>
{
	using Arguments = tuple<Args...>;
};

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

export template <typename Class, typename Method, typename... Args>
auto TickMainSpaceDll(Class* obj, const char* methodName, Method method, Args... args)
{
	using FuncSignature = decltype(method);
	using ArgsTuple = typename MemberFunctionArgs<FuncSignature>::Arguments;
	using RetType = typename MemberFunctionReturnType<Method>::RetType;
	typedef RetType(*MethodSign)(Class*, ArgsTuple);

	string className = typeid(Class).name();
	size_t pos = className.find(" ");
	if (pos != string::npos)
	{
		className = className.substr(pos + 1);
	}

	string fullFuncName = className + "_" + methodName;

	static unordered_map<string, void*> cache;

	auto it = cache.find(fullFuncName);
	if (it != cache.end())
	{
		MethodSign pFuncTyped = reinterpret_cast<MethodSign>(it->second);
		return pFuncTyped(obj, make_tuple(forward<Args>(args)...));
	}

	if (HMODULE hModule = GetModuleHandle(NULL))
	{
		if (void* pFunc = GetProcAddress(hModule, fullFuncName.c_str()))
		{
			cache[fullFuncName] = pFunc;
			MethodSign pFuncTyped = reinterpret_cast<MethodSign>(pFunc);
			return pFuncTyped(obj, make_tuple(forward<Args>(args)...));
		}
	}

	return DefaultReturnValue<RetType>::get();
}
