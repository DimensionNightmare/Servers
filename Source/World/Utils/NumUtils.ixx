export module NumUtils;

import std.compat;

class LockFreeSnowflake
{
private:
	// 各部分位数配置
	static constexpr uint64_t TIMESTAMP_BITS = 41;
	static constexpr uint64_t DATACENTER_BITS = 5;
	static constexpr uint64_t WORKER_BITS = 5;
	static constexpr uint64_t SEQUENCE_BITS = 12;

	// 最大值计算
	static constexpr uint64_t MAX_DATACENTER = (1ULL << DATACENTER_BITS) - 1;
	static constexpr uint64_t MAX_WORKER = (1ULL << WORKER_BITS) - 1;
	static constexpr uint64_t MAX_SEQUENCE = (1ULL << SEQUENCE_BITS) - 1;

	// 纪元时间（2020-01-01）
	static constexpr uint64_t EPOCH = 1577836800000ULL;

	// 原子状态变量
	std::atomic<uint64_t> last_timestamp_{ 0 };
	std::atomic<uint64_t> sequence_{ 0 };

	// 节点配置
	const uint64_t datacenter_id_;
	const uint64_t worker_id_;
	const uint64_t id_shift_;

public:
	LockFreeSnowflake(uint64_t datacenter, uint64_t worker)
		: datacenter_id_(datacenter),
		worker_id_(worker),
		id_shift_(DATACENTER_BITS + WORKER_BITS + SEQUENCE_BITS)
	{
		if (datacenter > MAX_DATACENTER)
		{
			throw std::invalid_argument("Datacenter ID exceeds maximum");
		}
		if (worker > MAX_WORKER)
		{
			throw std::invalid_argument("Worker ID exceeds maximum");
		}
	}

	uint64_t nextId()
	{
		uint64_t timestamp;
		uint64_t current_sequence;
		uint64_t new_sequence;
		uint64_t expected_timestamp;

		while (true)
		{
			// 读取当前状态
			expected_timestamp = last_timestamp_.load(std::memory_order_relaxed);
			timestamp = currentTimestamp();

			// 处理时钟回拨
			if (timestamp < expected_timestamp)
			{
				throw std::runtime_error("Clock moved backwards");
			}

			// 计算序列号
			if (timestamp == expected_timestamp)
			{
				current_sequence = sequence_.load(std::memory_order_relaxed);
				new_sequence = (current_sequence + 1) & MAX_SEQUENCE;

				if (new_sequence == 0)
				{ // 序列号耗尽
					timestamp = waitNextMs(expected_timestamp);
					continue;            // 重新尝试
				}
			}
			else
			{
				new_sequence = 0;       // 新时间戳重置序列号
			}

			// 原子更新状态
			if (last_timestamp_.compare_exchange_weak(
				expected_timestamp,
				timestamp,
				std::memory_order_relaxed))
			{

				// 更新序列号
				sequence_.store(new_sequence, std::memory_order_relaxed);
				break;
			}

			// 如果CAS失败，其他线程已经更新状态，循环重试
			std::this_thread::yield(); // 主动让出CPU
		}

		return ((timestamp - EPOCH) << id_shift_)
			| (datacenter_id_ << (WORKER_BITS + SEQUENCE_BITS))
			| (worker_id_ << SEQUENCE_BITS)
			| new_sequence;
	}

private:
	// 获取当前时间戳（毫秒）
	uint64_t currentTimestamp() const
	{
		return static_cast<uint64_t>(
			std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()
			).count()
			);
	}

	// 等待下一毫秒
	uint64_t waitNextMs(uint64_t last) const
	{
		uint64_t timestamp;
		do
		{
			std::this_thread::sleep_for(std::chrono::microseconds(100));
			timestamp = currentTimestamp();
		} while (timestamp <= last);
		return timestamp;
	}
};

export LockFreeSnowflake SFIdGenerator(0, 0); // dynamic initializer