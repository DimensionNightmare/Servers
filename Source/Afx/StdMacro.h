#pragma once

#include <tuple>

using namespace std;

#define DNPrint(code, level, fmt, ...) LoggerPrint(level, code, __FUNCTION__, fmt, ##__VA_ARGS__)

#define DBSelectOne(obj, name) .SelectOne(#name, [&obj]() { return obj.name(); })
#define DBSelectCond(obj, name, cond, splicing) .SelectCond(#name, cond, splicing, [&obj]() { return obj.name(); })
#define DBUpdate(obj, name) .Update(obj, #name, [&obj]() { return obj.name(); })
#define DBUpdateCond(obj, name, cond, splicing) .UpdateCond(#name, cond, splicing, [&obj]() { return obj.name(); })
#define DBDeleteCond(obj, name, cond, splicing) .DeleteCond(#name, cond, splicing, [&obj]() { return obj.name(); })

// key method
#define DBUpdateByKey(obj, name) .UpdateByKey(#name, [&obj]() { return obj.name(); })
#define DBSelectByKey(obj, name) .SelectByKey(#name, [&obj]() { return obj.name(); })

template <typename Method>
struct MemberFunctionArgs;

template <typename R, typename Class, typename... Args>
struct MemberFunctionArgs<R(Class::*)(Args...)>
{
	using Arguments = tuple<Args...>;
};

#define REGIST_MAINSPACE_SIGN_FUNCTION(classname, methodname)\
	using methodname##_Sign = decltype(&classname::methodname);\
	using methodname##_Args = typename MemberFunctionArgs<methodname##_Sign>::Arguments;\
	__declspec(dllexport) auto classname##_##methodname(classname *obj, methodname##_Args args)\
	{\
		return apply([&obj](auto &&...unpack) { return obj->methodname(forward<decltype(unpack)>(unpack)...); }, args);\
	}

#define TICK_MAINSPACE_SIGN_FUNCTION(Class, Method, Object, ...) \
	TickMainSpaceDll(static_cast<Class*>(Object), #Method, &Class::Method, __VA_ARGS__)

#define ASSERT(expr)\
	if (!(expr)) {abort();}
