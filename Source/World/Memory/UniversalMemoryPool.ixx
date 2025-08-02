module;

export module UniversalMemoryPool;

import std;

export class UniversalMemoryPool
{
	struct AllocationRecord
	{
		size_t size;
		size_t type_hash;
		std::source_location location;
	};

	struct DeferredDeallocation
	{
		void* memory;
		size_t size;
		size_t type_hash;
	};

public:
	UniversalMemoryPool(size_t pool_size = 1024 * 1024 * 256) : iPoolSize(pool_size)
	{
		pPool = static_cast<char*>(::operator new(iPoolSize));
		mFreeBlocks.emplace(pPool, iPoolSize);
	}

	~UniversalMemoryPool()
	{
		CheckLeaks();
		::operator delete(pPool);
	}

	template<typename T, typename... Args>
	std::shared_ptr<T> Allocate(Args&&... args,
		const std::source_location& loc = std::source_location::current()
	)
	{
		return AllocateImpl<T>(loc, std::forward<Args>(args)...);
	}

	void PrintLeaks(){ CheckLeaks(); }

private:
	// 统一的实现函数
	template<typename T, typename... Args>
	std::shared_ptr<T> AllocateImpl(
		const std::source_location& loc,
		Args&&... args
	)
	{
		AllocationScope scope(this);
		const size_t size = std::bit_ceil(sizeof(T));
		void* raw_memory = AllocateRaw(size);

		size_t type_hash = typeid(T).hash_code();
		scope.RecordAllocation(raw_memory, size, type_hash);
		std::memset(raw_memory, 0, size);

		try
		{
			T* object_ptr = new(raw_memory) T(std::forward<Args>(args)...);

			// 记录分配信息（包含源代码位置）
			RecordAllocationInfo(raw_memory, size, type_hash, loc);

			return std::shared_ptr<T>(
				object_ptr,
				[this, raw_memory, size, type_hash](T* ptr)
				{
					ptr->~T();
					DeferDeallocation(raw_memory, size, type_hash);
				}
			);
		}
		catch (...)
		{
			scope.RollbackAllocation();
			throw;
		}
	}

	class AllocationScope
	{
	public:
		AllocationScope(UniversalMemoryPool* pool) : pool(pool)
		{
			if (GetNestingLevel() == 0 && pool)
			{
				std::unique_lock lock(pool->oMtx);
				pool->ProcessDeferredDeallocations();
			}
			GetNestingLevel()++;
		}

		~AllocationScope()
		{
			if (--GetNestingLevel() == 0 && pool)
			{
				std::unique_lock lock(pool->oMtx);
				pool->ProcessDeferredDeallocations();
			}
		}

		bool IsTopLevel() const
		{
			return GetNestingLevel() == 1;
		}

		void RecordAllocation(void* memory, size_t size, size_t type_hash)
		{
			this->memory = memory;
			this->size = size;
			this->type_hash = type_hash;
		}

		void RollbackAllocation()
		{
			if (memory && pool)
			{
				pool->RollbackAllocation(memory, size);
				memory = nullptr;
			}
		}

		static size_t& GetNestingLevel()
		{
			thread_local size_t level = 0;
			return level;
		}

	private:
		UniversalMemoryPool* pool;
		void* memory = nullptr;
		size_t size = 0;
		size_t type_hash = 0;
	};

	// 记录分配信息（包含源代码位置）
	void RecordAllocationInfo(
		void* memory,
		size_t size,
		size_t type_hash,
		const std::source_location& loc
	)
	{
		std::unique_lock lock(oMtx);

		AllocationRecord record;
		record.size = size;
		record.type_hash = type_hash;
		record.location = loc;

		mAllocatedRecords[memory] = record;
	}

	void* AllocateRaw(size_t size)
	{
		std::unique_lock lock(oMtx);
		size = std::bit_ceil(size);

		auto it = FindFreeBlock(size);
		if (it == mFreeBlocks.end())
		{
			CoalesceFreeBlocks();
			it = FindFreeBlock(size);
		}

		if (it == mFreeBlocks.end())
		{
			ExpandPool(size * 2);
			it = FindFreeBlock(size);
		}

		if (it == mFreeBlocks.end())
		{
			throw std::bad_alloc();
		}

		char* addr = it->first;
		size_t block_size = it->second;
		void* raw_memory = addr;

		if (block_size > size + Alignment)
		{
			mFreeBlocks.emplace(addr + size, block_size - size);
		}
		else
		{
			size = block_size;
		}

		mFreeBlocks.erase(it);
		return raw_memory;
	}

	void ExpandPool(size_t additional_size)
	{
		char* new_pool = static_cast<char*>(::operator new(additional_size));
		mFreeBlocks.emplace(new_pool, additional_size);
		allocatedChunks.push_back({ new_pool, additional_size });
		iPoolSize += additional_size;
	}

	void RollbackAllocation(void* raw_memory, size_t size)
	{
		std::unique_lock lock(oMtx);
		mFreeBlocks.emplace(static_cast<char*>(raw_memory), size);
		mAllocatedRecords.erase(raw_memory);
	}

	void DeferDeallocation(void* raw_memory, size_t size, size_t type_hash)
	{
		{
			std::lock_guard lock(deferredMutex);
			deferredDeallocations.push_back({ raw_memory, size, type_hash });
		}

		if (AllocationScope::GetNestingLevel() == 0)
		{
			ProcessDeferredDeallocations();
		}
	}

	void ProcessDeferredDeallocations()
	{
		std::unique_lock lock(oMtx, std::defer_lock);
		std::list<DeferredDeallocation> deferred;

		{
			std::lock_guard def_lock(deferredMutex);
			deferred.swap(deferredDeallocations);
		}

		for (auto& item : deferred)
		{
			if (!lock.owns_lock()) lock.lock();
			DeallocateImpl(item.memory, item.size, item.type_hash);
		}
	}

	void DeallocateImpl(void* raw_memory, size_t size, size_t type_hash)
	{
		if (auto it = mAllocatedRecords.find(raw_memory); it != mAllocatedRecords.end())
		{
			auto& record = it->second;
			if (record.type_hash != 0 && record.type_hash != type_hash)
			{
				std::cerr << "Type mismatch during deallocation: "
					<< "Expected " << record.type_hash
					<< ", got " << type_hash << "\n";
			}

			MergeFreeBlocks(static_cast<char*>(raw_memory), size);
			mAllocatedRecords.erase(it);
		}
		else
		{
			std::cerr << "Attempt to deallocate unmanaged memory: "
				<< raw_memory << "\n";
		}
	}

	// 检查内存泄漏
	void CheckLeaks()
	{
		std::unique_lock lock(oMtx);

		if (!mAllocatedRecords.empty())
		{
			std::cerr << "\n\n*** MEMORY LEAK DETECTED ***\n";
			std::cerr << "Leaked " << mAllocatedRecords.size()
				<< " block(s) of memory\n";

			for (auto& [addr, record] : mAllocatedRecords)
			{
				std::cerr << "Leaked block at: " << addr << "\n"
					<< "  Size: " << record.size << " bytes\n"
					<< "  Type hash: " << record.type_hash << "\n"
					<< "  Allocation location:\n"
					<< "    File: " << record.location.file_name() << "\n"
					<< "    Function: " << record.location.function_name() << "\n"
					<< "    Line: " << record.location.line() << "\n"
					<< "    Column: " << record.location.column() << "\n\n";
			}

			std::cerr << "*** END OF LEAK REPORT ***\n\n";
		}
	}

	// 查找合适空闲块
	auto FindFreeBlock(size_t size)
	{
		for (auto it = mFreeBlocks.begin(); it != mFreeBlocks.end(); ++it)
		{
			if (it->second >= size)
			{
				return it;
			}
		}
		return mFreeBlocks.end();
	}

	void MergeFreeBlocks(char* addr, size_t size)
	{
		char* block_end = addr + size;

		// 向后合并
		auto next = mFreeBlocks.find(block_end);
		if (next != mFreeBlocks.end())
		{
			size += next->second;
			mFreeBlocks.erase(next);
		}

		// 向前合并
		for (auto it = mFreeBlocks.begin(); it != mFreeBlocks.end(); )
		{
			char* cur_addr = it->first;
			size_t cur_size = it->second;

			if (cur_addr + cur_size == addr)
			{
				size += cur_size;
				addr = cur_addr;
				it = mFreeBlocks.erase(it);
			}
			else
			{
				++it;
			}
		}

		mFreeBlocks[addr] = size;
	}

	void CoalesceFreeBlocks()
	{
		std::vector<std::pair<char*, size_t>> blocks;
		for (auto& block : mFreeBlocks)
		{
			blocks.push_back(block);
		}

		// 按地址排序
		std::sort(blocks.begin(), blocks.end(),
			[](const auto& a, const auto& b) { return a.first < b.first; });

		// 合并相邻块
		for (size_t i = 0; i < blocks.size() - 1; )
		{
			char* end = blocks[i].first + blocks[i].second;
			if (end == blocks[i + 1].first)
			{
				blocks[i].second += blocks[i + 1].second;
				blocks.erase(blocks.begin() + i + 1);
			}
			else
			{
				i++;
			}
		}

		// 更新空闲块表
		mFreeBlocks.clear();
		for (auto& block : blocks)
		{
			mFreeBlocks.insert(block);
		}
	}

	static constexpr size_t Alignment = alignof(std::max_align_t);

	char* pPool;
	size_t iPoolSize;
	std::map<char*, size_t> mFreeBlocks;
	std::unordered_map<void*, AllocationRecord> mAllocatedRecords;
	std::list<DeferredDeallocation> deferredDeallocations;
	std::recursive_mutex oMtx;
	std::mutex deferredMutex;

	// 分配的内存块记录
	std::vector<std::pair<char*, size_t>> allocatedChunks;
};
