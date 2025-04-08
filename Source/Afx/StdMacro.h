#pragma once

import std.compat;

#define DNPrintCode(code, ...) LoggerPrint(code, __FUNCTION__, ##__VA_ARGS__)

#define DNPrint(level, fmt, ...) LoggerPrint(level, fmt, __FUNCTION__, ##__VA_ARGS__)

#define DBSelectOne(obj, name) .SelectOne(#name, [&obj]() { return obj.name(); })
#define DBSelectCond(obj, name, cond, splicing) .SelectCond(#name, cond, splicing, [&obj]() { return obj.name(); })
#define DBUpdate(obj, name) .Update(obj, #name, [&obj]() { return obj.name(); })
#define DBUpdateCond(obj, name, cond, splicing) .UpdateCond(#name, cond, splicing, [&obj]() { return obj.name(); })
#define DBDeleteCond(obj, name, cond, splicing) .DeleteCond(#name, cond, splicing, [&obj]() { return obj.name(); })

// key method
#define DBUpdateByKey(obj, name) .UpdateByKey(#name, [&obj]() { return obj.name(); })
#define DBSelectByKey(obj, name) .SelectByKey(#name, [&obj]() { return obj.name(); })

#define TICK_MAINSPACE_SIGN_FUNCTION(Class, Method, Object, ...) \
	TickMainSpaceDll(Object, #Method, &Class::Method, __VA_ARGS__)

#define ASSERT(expr)\
	if (!(expr)) {abort();}
