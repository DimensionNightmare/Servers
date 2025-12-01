export module NumUtils;

import std.compat;

/// @brief Lock-free Snowflake ID generator using modern C++23 features
class LockFreeSnowflake
{
private:
	// Configuration using static constexpr
	static constexpr size_t TIMESTAMP_BITS = 41;
	static constexpr size_t DATACENTER_BITS = 5;
	static constexpr size_t WORKER_BITS = 5;
	static constexpr size_t SEQUENCE_BITS = 12;

	// Maximum values calculated at compile-time
	static constexpr size_t MAX_DATACENTER = (1ULL << DATACENTER_BITS) - 1;
	static constexpr size_t MAX_WORKER = (1ULL << WORKER_BITS) - 1;
	static constexpr size_t MAX_SEQUENCE = (1ULL << SEQUENCE_BITS) - 1;

	// Epoch timestamp (2020-01-01)
	static constexpr size_t EPOCH = 1577836800000ULL;

	// Atomic state variables
	std::atomic<size_t> last_timestamp_{ 0 };
	std::atomic<size_t> sequence_{ 0 };

	// Node configuration (immutable after construction)
	const size_t datacenter_id_;
	const size_t worker_id_;
	const size_t id_shift_;

public:
	LockFreeSnowflake(size_t datacenter, size_t worker)
		: datacenter_id_(datacenter),
		  worker_id_(worker),
		  id_shift_(DATACENTER_BITS + WORKER_BITS + SEQUENCE_BITS)
	{
		if (datacenter > MAX_DATACENTER) [[unlikely]]
		{
			throw std::invalid_argument("Datacenter ID exceeds maximum");
		}
		if (worker > MAX_WORKER) [[unlikely]]
		{
			throw std::invalid_argument("Worker ID exceeds maximum");
		}
	}

	// Deleted copy/move operations for safety
	LockFreeSnowflake(const LockFreeSnowflake&) = delete;
	LockFreeSnowflake& operator=(const LockFreeSnowflake&) = delete;
	LockFreeSnowflake(LockFreeSnowflake&&) = delete;
	LockFreeSnowflake& operator=(LockFreeSnowflake&&) = delete;

	[[nodiscard]] size_t nextId()
	{
		size_t timestamp;
		size_t new_sequence;
		size_t expected_timestamp;

		while (true)
		{
			// Read current state with relaxed ordering
			expected_timestamp = last_timestamp_.load(std::memory_order_relaxed);
			timestamp = currentTimestamp();

			// Handle clock drift
			if (timestamp < expected_timestamp) [[unlikely]]
			{
				throw std::runtime_error("Clock moved backwards");
			}

			// Calculate sequence number
			if (timestamp == expected_timestamp)
			{
				size_t current_sequence = sequence_.load(std::memory_order_relaxed);
				new_sequence = (current_sequence + 1) & MAX_SEQUENCE;

				if (new_sequence == 0) [[unlikely]] // Sequence exhausted
				{
					timestamp = waitNextMs(expected_timestamp);
					continue;
				}
			}
			else
			{
				new_sequence = 0; // Reset sequence for new timestamp
			}

			// Atomic update using compare-exchange
			if (last_timestamp_.compare_exchange_weak(
				expected_timestamp,
				timestamp,
				std::memory_order_relaxed))
			{
				sequence_.store(new_sequence, std::memory_order_relaxed);
				break;
			}

			// CAS failed, yield and retry
			std::this_thread::yield();
		}

		return ((timestamp - EPOCH) << id_shift_)
			| (datacenter_id_ << (WORKER_BITS + SEQUENCE_BITS))
			| (worker_id_ << SEQUENCE_BITS)
			| new_sequence;
	}

private:
	[[nodiscard]] size_t currentTimestamp() const noexcept
	{
		using namespace std::chrono;
		return static_cast<size_t>(
			duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count()
		);
	}

	[[nodiscard]] size_t waitNextMs(size_t last) const noexcept
	{
		size_t timestamp;
		do
		{
			std::this_thread::sleep_for(std::chrono::microseconds(100));
			timestamp = currentTimestamp();
		} while (timestamp <= last);
		return timestamp;
	}
};

export LockFreeSnowflake SFIdGenerator(0, 0); // dynamic initializer