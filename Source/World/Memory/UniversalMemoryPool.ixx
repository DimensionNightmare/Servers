export module UniversalMemoryPool;

import std.compat;

// import Logger;

export class UniversalMemoryPool
{
	static constexpr size_t MaxCachedSize = 4096; // 最大缓存块大小

	static constexpr size_t BucketCount = 16; // 桶数量

	// 分配记录
	struct AllocationRecord
	{
		size_t size;
		std::source_location location;
	};

	// 大小分类桶
	struct SizeBucket
	{
		std::shared_mutex mutex;
		std::vector<void*> blocks;
	};

public:
	using Ptr = std::shared_ptr<UniversalMemoryPool>;

	UniversalMemoryPool(size_t pool_size = 1024 * 1024 * 32) : iPoolSize(pool_size)
	{
		pPool = ::operator new(iPoolSize);
		AddToFreeList(pPool, iPoolSize);
	}

	~UniversalMemoryPool()
	{
		// 释放桶
		FreeBucket();

		// 检查泄漏
		CheckLeaks();

		// 释放主内存池
		::operator delete(pPool);
	}

	std::string GetMemoryRecordInfo(void* raw_memory)
	{
		auto it = mAllocatedRecords.find(raw_memory);
		if(it != mAllocatedRecords.end())
		{
			const auto& location = it->second.location;

			return std::format("{},{},{}", location.file_name(), location.line(), location.function_name());
		}

		return "";
	}

	void SetMemoryRecordInfo(void* raw_memory, std::source_location&& location)
	{
		auto it = mAllocatedRecords.find(raw_memory);
		if(it != mAllocatedRecords.end())
		{
			it->second.location = std::move(location);
		}
	}

	template<typename T, typename... Args>
	// requires std::invocable<T,Args...>
	std::shared_ptr<T> Allocate(Args&&... args)
	{
		constexpr size_t size = std::bit_ceil(sizeof(T));
		void* raw_memory = nullptr;

		try
		{
			// 尝试快速分配
			raw_memory = TryFastAllocation(size);
			if (!raw_memory)
			{
				raw_memory = AllocateRaw(size);
			}

			if (raw_memory)
			{
				// 记录分配信息（包含源代码位置）
				RecordAllocationInfo(raw_memory, size);

				T* object_ptr = new (raw_memory) T(std::forward<Args>(args)...);

				return {
					object_ptr
					, [this, raw_memory, size](T* ptr)
					{
						ptr->~T();
						RollbackAllocation(raw_memory, size);
					}
				};
			}

			// return std::shared_ptr<T>(new T(std::forward<Args>(args)...));
			return nullptr;
		}
		catch (...)
		{
			if (raw_memory)
			{
				RollbackAllocation(raw_memory, size);
			}
			throw;
		}
	}

protected:
	// 尝试快速分配
	void* TryFastAllocation(size_t size)
	{
		// 只尝试缓存小到中等大小的块
		if (size > MaxCachedSize)
		{
			return nullptr;
		}

		// 获取桶索引
		size_t bucket_index = GetBucketIndex(size);

		// 尝试从桶中快速获取
		{
			std::unique_lock lock(mSizeBuckets[bucket_index].mutex);
			
			if (!mSizeBuckets[bucket_index].blocks.empty())
			{
				void* addr = mSizeBuckets[bucket_index].blocks.back();
				mSizeBuckets[bucket_index].blocks.pop_back();
				return addr;
			}
		}
		return nullptr;
	}

	// 核心分配函数
	void* AllocateRaw(size_t size)
	{
		void* addr = nullptr;
		
		std::unique_lock lock(oMainMutex);
		if (auto it = FindFreeBlock(size); it != mFreeBlocks.end())
		{
			addr = AllocateFromIterator(it, size);
		}
		
		return addr;
	}

	// 辅助函数：从迭代器分配（修复死锁）
	void* AllocateFromIterator(auto it, size_t size)
	{
		void* addr = it->first;
		size_t block_size = it->second;
		void* raw_memory = addr;

		if (block_size >= size)
		{
			// 将剩余块添加到合适的空闲链表
			size_t remaining = block_size - size;
			void* remaining_addr = static_cast<uint8_t*>(addr) + size;

			// 直接添加剩余块，避免调用AddToFreeList
			if (remaining <= MaxCachedSize)
			{
				size_t bucket_index = GetBucketIndex(remaining);
				std::unique_lock lock(mSizeBuckets[bucket_index].mutex);
				
				mSizeBuckets[bucket_index].blocks.push_back(remaining_addr);
			}
			else
			{
				mFreeBlocks.emplace(remaining_addr, remaining);
			}
		}

		mFreeBlocks.erase(it);
		return raw_memory;
	}

	// 添加块到空闲链表（修复死锁）
	bool AddToFreeList(void* addr, size_t size)
	{
		// 如果大小在缓存范围内，添加到桶中
		if (size <= MaxCachedSize)
		{
			size_t bucket_index = GetBucketIndex(size);
			std::unique_lock lock(mSizeBuckets[bucket_index].mutex);
			
			mSizeBuckets[bucket_index].blocks.push_back(addr);

			return true;
		}
		// 否则添加到主内存池
		else
		{
			std::unique_lock lock(oMainMutex);
			
			mFreeBlocks.emplace(addr, size);
		}

		return false;
	}

	// 获取桶索引
	size_t GetBucketIndex(size_t size)
	{
		// 使用位宽计算桶索引
		return std::min<size_t>(
			std::bit_width(size),
			BucketCount - 1
		);
	}

	// 记录分配信息
	void RecordAllocationInfo(void* memory, size_t size)
	{
		
		std::unique_lock lock(oRecordMutex);

		AllocationRecord record;
		record.size = size;

		mAllocatedRecords[memory] = record;
	}

	void RollbackAllocation(void* raw_memory, size_t size)
	{
		if(!AddToFreeList(raw_memory, size)) // 如果没加在桶中
		{
			std::unique_lock lock(oRecordMutex);
			
			auto it = mAllocatedRecords.find(raw_memory);
			if (it != mAllocatedRecords.end())
			{
				mAllocatedRecords.erase(it);
			}
			// *** maybe second BucketFree
			// else
			// {
			// 	std::cerr << "Warning: Attempted to rollback untracked memory at "
			// 		<< std::format("{}", raw_memory) << "\n";
			// }
		}
	}

	// 检查内存泄漏
	void CheckLeaks()
	{
		std::shared_lock lock(oRecordMutex);

		if (!mAllocatedRecords.empty())
		{
			std::cerr << "\n\n*** MEMORY LEAK DETECTED ***\n";
			std::cerr << "Leaked " << mAllocatedRecords.size()
				<< " block(s) of memory\n";

			for (auto& [addr, record] : mAllocatedRecords)
			{
				std::cerr << "Leaked block at: " << std::format("{}", addr) << "\n"
					<< "  Size: " << record.size << " bytes\n"
					<< "  Allocation location:\n"
					<< "    File: " << record.location.file_name() << ":" << record.location.line() << "\n";
			}

			std::cerr << "*** END OF LEAK REPORT ***\n\n";
		}
	}

	// 查找合适空闲块（优化版）
	auto FindFreeBlock(size_t size)
	{
		// 使用高效算法查找最佳匹配块
		auto best_fit = mFreeBlocks.end();

		for (auto it = mFreeBlocks.begin(); it != best_fit; ++it)
		{
			if (it->second >= size)
			{
				return it;
			}
		}

		return best_fit;
	}

	void FreeBucket()
	{
		for(SizeBucket& bucket : mSizeBuckets)
		{
			for(void* block : bucket.blocks)
			{
				mAllocatedRecords.erase(block);
			}
		}
	}

protected:

	void* pPool = nullptr;
	size_t iPoolSize = 0;

	// 主内存池
	std::unordered_map<void*, size_t> mFreeBlocks;
	std::shared_mutex oMainMutex;
	std::shared_mutex oRecordMutex;

	// 大小分类桶
	std::array<SizeBucket, BucketCount> mSizeBuckets;

	// 分配记录
	std::unordered_map<void*, AllocationRecord> mAllocatedRecords;
};
