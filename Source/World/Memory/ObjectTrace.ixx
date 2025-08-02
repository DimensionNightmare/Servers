module;
export module ObjectTrace;

// import ECSW;

// void* operator new(size_t size)
// {
// 	if(MemPool) return MemPool->AllocateRaw(size);

// 	return std::malloc(size);
// 	// return MemPool->AllocateRaw(size);
// }

// void* operator new[](size_t size)
// {
// 	if(MemPool) return MemPool->AllocateRaw(size);
// 	return std::malloc(size);
// 	// return MemPool->AllocateRaw(size);
// }

// void operator delete(void* pointer)
// {
// 	if(MemPool) MemPool->DeallocateRaw(pointer);
// 	else std::free(pointer);
// 	// return free(pointer);
// 	// return MemPool->DeallocateRaw(pointer);
// }

// void operator delete[](void* pointer)
// {
// 	if(MemPool) MemPool->DeallocateRaw(pointer);
// 	else std::free(pointer);
// 	// return free(pointer);
// 	// return MemPool->DeallocateRaw(pointer);
// }