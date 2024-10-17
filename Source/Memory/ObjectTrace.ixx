module;
#include <stdlib.h>
#include "StdMacro.h"
export module ObjectTrace;

void* operator new(size_t size)
{
	return malloc(size);
}

void* operator new[](size_t size)
{
	return malloc(size);
}

void operator delete(void* pointer)
{
	return free(pointer);
}

void operator delete[](void* pointer)
{
	return free(pointer);
}