export module UniversalMemoryPool;

import std.compat;

// import Logger;

std::atomic<int> allocint(0); 

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
		std::vector<char*> blocks;
	};

	// 锁统计数据结构
	struct LockStats
	{
		std::atomic_uint64_t totalWaitNs = 0;  // 总等待时间（纳秒）
		std::atomic_uint64_t maxWaitNs = 0;     // 最大单次等待时间
		std::atomic_uint64_t lockCount = 0;     // 锁获取次数
	};

	// 锁统计收集器
	class LockProfiler
	{
	public:
		LockProfiler(LockStats& stats) : stats(stats)
		{
			start = std::chrono::steady_clock::now();
		}

		~LockProfiler()
		{
			auto end = std::chrono::steady_clock::now();
			uint64_t duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

			stats.totalWaitNs += duration;
			stats.lockCount++;

			// 原子更新最大值
			uint64_t currentMax = stats.maxWaitNs.load();
			while (duration > currentMax &&
				!stats.maxWaitNs.compare_exchange_weak(currentMax, duration))
			{
				// 循环直到更新成功
			}
		}

	private:
		LockStats& stats;
		std::chrono::steady_clock::time_point start;
	};

	// 锁统计数据
	LockStats mainLockStats;
	std::array<LockStats, BucketCount> bucketLockStats;
	std::atomic_bool enableProfiling{ true };

public:
	UniversalMemoryPool(size_t pool_size = 1024 * 1024 * 128) : iPoolSize(pool_size)
	{
		pPool = static_cast<char*>(::operator new(iPoolSize));
		AddToFreeList(pPool, iPoolSize);
	}

	~UniversalMemoryPool()
	{
		// 检查泄漏
		CheckLeaks();

		// 释放主内存池
		::operator delete(pPool);
	}

	template<typename T, typename... Args>
	std::shared_ptr<T> Allocate(Args&&... args
		,const std::source_location& loc = std::source_location::current()
	)
	{
		constexpr size_t size = std::bit_ceil(sizeof(T));
		char* raw_memory = nullptr;

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
				RecordAllocationInfo(raw_memory, size, loc);

				allocint++;
				
				T* object_ptr = new(raw_memory) T(std::forward<Args>(args)...);

				return std::shared_ptr<T>(
					object_ptr,
					[this, raw_memory, size](T* ptr)
					{
						allocint--;
						ptr->~T();
						std::cout << std::format("{}", static_cast<void*>(ptr)) << std::endl;
						RollbackAllocation(raw_memory, size);
					}
				);
			}

			return std::shared_ptr<T>(new T(std::forward<Args>(args)...));
		}
		catch (...)
		{
			if (raw_memory)
			{
				std::cout << std::format( "{}", static_cast<void*>(raw_memory))  << std::endl;;
				RollbackAllocation(raw_memory, size);
			}
			throw;
		}
	}
	
	void PrintLeaks() { CheckLeaks(); }

	void EnableProfiling(bool enable) { enableProfiling = enable; }

	// 获取锁统计信息
	void PrintLockStats(int core) const
	{
		if (!enableProfiling)
		{
			std::cout << "Profiling is disabled\n";
			return;
		}

		std::cout << "\n===== Lock Wait Time Statistics =====\n";

		// 主锁统计
		std::cout << "Main Lock:\n";
		PrintSingleLockStats(mainLockStats, core);

		// 桶锁统计
		for (size_t i = 0; i < BucketCount; i++)
		{
			std::cout << "Bucket " << i << ":\n";
			PrintSingleLockStats(bucketLockStats[i], core);
		}
	}

private:
	// 尝试快速分配
	char* TryFastAllocation(size_t size)
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
			std::unique_lock lock(sizeBuckets[bucket_index].mutex);
			// auto lock = GetLock(sizeBuckets[bucket_index].mutex, bucketLockStats[bucket_index]);
			if (!sizeBuckets[bucket_index].blocks.empty())
			{
				char* addr = sizeBuckets[bucket_index].blocks.back();
				sizeBuckets[bucket_index].blocks.pop_back();
				return addr;
			}
		}
		return nullptr;
	}

	// 核心分配函数
	char* AllocateRaw(size_t size)
	{
		char* addr = nullptr;

		// auto lock = GetLock(mainMutex, mainLockStats);
		{
			std::unique_lock lock(mainMutex);
			if (auto it = FindFreeBlock(size); it != mFreeBlocks.end())
			{
				addr = AllocateFromIterator(it, size);
			}
		}
		
		return addr;
	}

	// 辅助函数：从迭代器分配（修复死锁）
	char* AllocateFromIterator(auto it, size_t size)
	{
		char* addr = it->first;
		size_t block_size = it->second;
		char* raw_memory = addr;

		if (block_size >= size)
		{
			// 将剩余块添加到合适的空闲链表
			size_t remaining = block_size - size;
			char* remaining_addr = addr + size;

			// 直接添加剩余块，避免调用AddToFreeList
			if (remaining <= MaxCachedSize)
			{
				size_t bucket_index = GetBucketIndex(remaining);
				std::unique_lock lock(sizeBuckets[bucket_index].mutex);
				// auto lock = GetLock(sizeBuckets[bucket_index].mutex, bucketLockStats[bucket_index]);
				sizeBuckets[bucket_index].blocks.push_back(remaining_addr);
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
	void AddToFreeList(char* addr, size_t size)
	{
		// 如果大小在缓存范围内，添加到桶中
		if (size <= MaxCachedSize)
		{
			size_t bucket_index = GetBucketIndex(size);
			std::unique_lock lock(sizeBuckets[bucket_index].mutex);
			// auto lock = GetLock(sizeBuckets[bucket_index].mutex, bucketLockStats[bucket_index]);
			sizeBuckets[bucket_index].blocks.push_back(addr);
		}
		// 否则添加到主内存池
		else
		{
			std::unique_lock lock(mainMutex);
			// auto lock = GetLock(mainMutex, mainLockStats);
			mFreeBlocks.emplace(addr, size);
		}
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
	void RecordAllocationInfo(
		char* memory,
		size_t size,
		const std::source_location& loc
	)
	{
		// auto lock = GetLock(recordMutex, mainLockStats);
		std::unique_lock lock(recordMutex);

		AllocationRecord record;
		record.size = size;
		record.location = loc;

		mAllocatedRecords[memory] = record;
	}

	void RollbackAllocation(char* raw_memory, size_t size)
	{
		AddToFreeList(raw_memory, size);
		{
			std::unique_lock lock(recordMutex);
			// auto lock = GetLock(recordMutex, mainLockStats);
			mAllocatedRecords.erase(raw_memory);
		}
	}

	// 检查内存泄漏
	void CheckLeaks()
	{
		std::shared_lock lock(recordMutex);
		// auto lock = GetLock(recordMutex, mainLockStats);

		if (!mAllocatedRecords.empty())
		{
			std::cerr << "\n\n*** MEMORY LEAK DETECTED ***\n";
			std::cerr << "Leaked " << mAllocatedRecords.size() << " alloc/free " << allocint
				<< " block(s) of memory\n";

			for (auto& [addr, record] : mAllocatedRecords)
			{
				std::cerr << "Leaked block at: " << static_cast<void*>(addr) << "\n"
					<< "  Size: " << record.size << " bytes\n"
					<< "  Allocation location:\n"
					<< "    File: " << record.location.file_name() << "\n"
					<< "    Function: " << record.location.function_name() << "\n"
					<< "    Line: " << record.location.line() << "\n"
					<< "    Column: " << record.location.column() << "\n\n";
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

	// 打印单个锁的统计信息
	void PrintSingleLockStats(const LockStats& stats, int core) const
	{
		uint64_t count = stats.lockCount.load();
		if (count == 0)
		{
			std::cout << "  No locks acquired\n";
			return;
		}

		uint64_t totalNs = stats.totalWaitNs.load();
		uint64_t maxNs = stats.maxWaitNs.load();

		std::cout << "  Lock count: " << count << "\n";
		std::cout << "  Total wait time: " << FormatNs(totalNs) << "\n";
		std::cout << "  Total core wait time: " << FormatNs(totalNs / core) << "\n";
		std::cout << "  Average wait time: " << FormatNs(totalNs / count) << "\n";
		std::cout << "  Max wait time: " << FormatNs(maxNs) << "\n";
	}

	// 格式化纳秒时间为易读格式
	std::string FormatNs(uint64_t ns) const
	{
		if (ns < 1000) return std::to_string(ns) + " ns";
		if (ns < 1000000) return std::to_string(ns / 1000.0) + " μs";
		if (ns < 1000000000) return std::to_string(ns / 1000000.0) + " ms";
		return std::to_string(ns / 1000000000.0) + " s";
	}

	// 带统计的锁获取
	template<typename Mutex>
	auto GetLock(Mutex& mutex, LockStats& stats)
	{
		if (enableProfiling)
		{
			LockProfiler profiler(stats);
			return std::unique_lock<Mutex>(mutex);
		}
		return std::unique_lock<Mutex>(mutex);
	}


	char* pPool = nullptr;
	size_t iPoolSize = 0;

	// 主内存池
	std::map<char*, size_t> mFreeBlocks;
	std::shared_mutex mainMutex;
	std::shared_mutex recordMutex;

	// 大小分类桶
	std::array<SizeBucket, BucketCount> sizeBuckets;

	// 分配记录
	std::unordered_map<char*, AllocationRecord> mAllocatedRecords;

	// 分配的内存块记录
	std::vector<std::pair<char*, size_t>> allocatedChunks;
};
