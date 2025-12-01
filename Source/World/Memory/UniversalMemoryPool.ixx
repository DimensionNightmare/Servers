export module UniversalMemoryPool;

import std.compat;

/// @brief Universal memory pool with thread-safe allocation and leak detection
export class UniversalMemoryPool
{
	static constexpr size_t MaxCachedSize = 4096; // Maximum cached block size
	static constexpr size_t BucketCount = 16;     // Number of size buckets

	/// @brief Allocation record for leak detection
	struct AllocationRecord
	{
		size_t size{};
		std::source_location location{};
	};

	/// @brief Size-based bucket for fast allocation
	struct SizeBucket
	{
		std::shared_mutex mutex;
		std::vector<void*> blocks;
	};

public:
	using Ptr = std::shared_ptr<UniversalMemoryPool>;
	using CVPtr = const Ptr&;

	explicit UniversalMemoryPool(size_t pool_size = 1024 * 1024 * 32) : iPoolSize(pool_size)
	{
		pPool = ::operator new(iPoolSize);
		AddToFreeList(pPool, iPoolSize);
	}

	~UniversalMemoryPool()
	{
		FreeBucket();
		CheckLeaks();
		::operator delete(pPool);
	}

	// Non-copyable, non-movable
	UniversalMemoryPool(const UniversalMemoryPool&) = delete;
	UniversalMemoryPool& operator=(const UniversalMemoryPool&) = delete;
	UniversalMemoryPool(UniversalMemoryPool&&) = delete;
	UniversalMemoryPool& operator=(UniversalMemoryPool&&) = delete;

	[[nodiscard]] std::string GetMemoryRecordInfo(void* raw_memory)
	{
		std::shared_lock lock(oRecordMutex);

		if (auto it = mAllocatedRecords.find(raw_memory); it != mAllocatedRecords.end())
		{
			const auto& location = it->second.location;
			return std::format("{},{},{}", location.file_name(), location.line(), location.function_name());
		}

		return {};
	}

	void SetMemoryRecordInfo(void* raw_memory, std::source_location&& location)
	{
		std::shared_lock lock(oRecordMutex);
		
		if (auto it = mAllocatedRecords.find(raw_memory); it != mAllocatedRecords.end())
		{
			it->second.location = std::move(location);
		}
	}

	template<typename T, typename... Args>
	[[nodiscard]] std::shared_ptr<T> Allocate(Args&&... args)
	{
		constexpr size_t size = std::bit_ceil(sizeof(T));
		void* raw_memory = nullptr;

		try
		{
			// Try fast allocation first
			raw_memory = TryFastAllocation(size);
			if (!raw_memory) [[unlikely]]
			{
				raw_memory = AllocateRaw(size);
			}

			if (raw_memory) [[likely]]
			{
				RecordAllocationInfo(raw_memory, size);

				T* object_ptr = new (raw_memory) T(std::forward<Args>(args)...);

				return {
					object_ptr, 
					[this, raw_memory, size](T* ptr) noexcept
					{
						ptr->~T();
						RollbackAllocation(raw_memory, size);
					}
				};
			}

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
	/// @brief Try fast allocation from size bucket
	[[nodiscard]] void* TryFastAllocation(size_t size) noexcept
	{
		if (size > MaxCachedSize) [[unlikely]]
		{
			return nullptr;
		}

		size_t bucket_index = GetBucketIndex(size);

		std::unique_lock lock(mSizeBuckets[bucket_index].mutex);
		
		if (!mSizeBuckets[bucket_index].blocks.empty()) [[likely]]
		{
			void* addr = mSizeBuckets[bucket_index].blocks.back();
			mSizeBuckets[bucket_index].blocks.pop_back();
			return addr;
		}

		return nullptr;
	}

	/// @brief Core allocation from main pool
	[[nodiscard]] void* AllocateRaw(size_t size)
	{
		void* addr = nullptr;
		
		std::unique_lock lock(oMainMutex);
		if (auto it = FindFreeBlock(size); it != mFreeBlocks.end())
		{
			addr = AllocateFromIterator(it, size);
		}
		
		return addr;
	}

	/// @brief Allocate from iterator with block splitting
	void* AllocateFromIterator(auto it, size_t size)
	{
		void* addr = it->first;
		size_t block_size = it->second;
		void* raw_memory = addr;

		if (block_size >= size) [[likely]]
		{
			size_t remaining = block_size - size;
			void* remaining_addr = static_cast<uint8_t*>(addr) + size;

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

	/// @brief Add block to free list
	bool AddToFreeList(void* addr, size_t size) noexcept
	{
		if (size <= MaxCachedSize)
		{
			size_t bucket_index = GetBucketIndex(size);
			std::unique_lock lock(mSizeBuckets[bucket_index].mutex);
			mSizeBuckets[bucket_index].blocks.push_back(addr);
			return true;
		}
		else
		{
			std::unique_lock lock(oMainMutex);
			mFreeBlocks.emplace(addr, size);
		}

		return false;
	}

	/// @brief Calculate bucket index using bit width
	[[nodiscard]] static constexpr size_t GetBucketIndex(size_t size) noexcept
	{
		return std::min<size_t>(std::bit_width(size), BucketCount - 1);
	}

	/// @brief Record allocation for leak detection
	void RecordAllocationInfo(void* memory, size_t size)
	{
		std::unique_lock lock(oRecordMutex);

		mAllocatedRecords[memory] = AllocationRecord{ .size = size };
	}

	/// @brief Rollback allocation on failure or deallocation
	void RollbackAllocation(void* raw_memory, size_t size) noexcept
	{
		if (!AddToFreeList(raw_memory, size))
		{
			std::unique_lock lock(oRecordMutex);
			mAllocatedRecords.erase(raw_memory);
		}
	}

	/// @brief Check for memory leaks on destruction
	void CheckLeaks() const
	{
		std::shared_lock lock(oRecordMutex);

		if (!mAllocatedRecords.empty()) [[unlikely]]
		{
			std::cerr << "\n\n*** MEMORY LEAK DETECTED ***\n";
			std::cerr << "Leaked " << mAllocatedRecords.size() << " block(s) of memory\n";

			for (const auto& [addr, record] : mAllocatedRecords)
			{
				std::cerr << "Leaked block at: " << std::format("{}", addr) << "\n"
					<< "  Size: " << record.size << " bytes\n"
					<< "  Allocation location:\n"
					<< "    File: " << record.location.file_name() << ":" << record.location.line() << "\n";
			}

			std::cerr << "*** END OF LEAK REPORT ***\n\n";
		}
	}

	/// @brief Find best-fit free block
	[[nodiscard]] auto FindFreeBlock(size_t size)
	{
		auto best_fit = mFreeBlocks.end();

		for (auto it = mFreeBlocks.begin(); it != best_fit; ++it)
		{
			if (it->second >= size) [[likely]]
			{
				return it;
			}
		}

		return best_fit;
	}

	/// @brief Free all bucket memory
	void FreeBucket()
	{
		for (const auto& bucket : mSizeBuckets)
		{
			for (void* block : bucket.blocks)
			{
				mAllocatedRecords.erase(block);
			}
		}
	}

protected:
	void* pPool = nullptr;
	size_t iPoolSize = 0;

	// Main memory pool
	std::unordered_map<void*, size_t> mFreeBlocks;
	mutable std::shared_mutex oMainMutex;
	mutable std::shared_mutex oRecordMutex;

	// Size buckets for fast allocation
	std::array<SizeBucket, BucketCount> mSizeBuckets;

	// Allocation records for leak detection
	std::unordered_map<void*, AllocationRecord> mAllocatedRecords;
};
