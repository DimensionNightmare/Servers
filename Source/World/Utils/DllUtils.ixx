module;

export module DllUtils;

import std;
import Platform;

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

export template <typename Method>
struct MemberFunctionArgs;

export template <typename R, typename Class, typename... Args>
struct MemberFunctionArgs<R(Class::*)(Args...)>
{
	using Arguments = std::tuple<Args...>;
};

export template <typename Method, typename... Args>
auto TickMainSpaceDll(void* obj, const char* classmethod, Method method, Args... args)
{
	using FuncSignature = decltype(method);
	using ArgsTuple = typename MemberFunctionArgs<FuncSignature>::Arguments;
	using RetType = typename MemberFunctionReturnType<Method>::RetType;
	typedef RetType(*MethodSign)(void*, ArgsTuple);

	std::string methodName = std::regex_replace(++classmethod, std::regex(R"(::)"), "_");

	// std::cout << typeid(RetType).name() << std::endl;

	static std::unordered_map<std::string, void*> cache;

	if (auto it = cache.find(methodName);it != cache.end())
	{
		MethodSign pFuncTyped = reinterpret_cast<MethodSign>(it->second);
		return pFuncTyped(obj, std::make_tuple(std::forward<Args>(args)...));
	}

	if (Platform::FuncHandle pFunc = Platform::GetProcAddress(nullptr, methodName.c_str()))
	{
		cache[methodName] = pFunc;
		MethodSign pFuncTyped = reinterpret_cast<MethodSign>(pFunc);
		return pFuncTyped(obj, std::forward_as_tuple(std::forward<Args>(args)...));
	}
	
	return DefaultReturnValue<RetType>::get();
}
