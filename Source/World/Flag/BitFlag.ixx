export module BitFlag;

import std.compat;

template <typename T>
concept HasMaxField = requires {
    { T::Max };
	requires std::to_underlying(T::Max) > 0;
};

export template<typename T>
requires HasMaxField<T>
class BitFlag
{
public:
	using NumType = std::underlying_type_t<T>;
	static constexpr NumType MaxValue = std::to_underlying(T::Max);

	constexpr void CheckBounds(this auto&& self, NumType flag) {
		if (flag >= MaxValue) [[unlikely]] {
			throw std::out_of_range("Flag value out of bounds");
		}
    }

	[[nodiscard]] constexpr bool HasFlag(this auto const& self, T flag) noexcept { 
		return self.oFlags.test(std::to_underlying(flag)); 
	}
	
	constexpr void SetFlag(this auto&& self, T flag) noexcept { 
		self.oFlags.set(std::to_underlying(flag)); 
	}
	
	constexpr void SetFlag(this auto&& self, NumType flag) { 
		self.CheckBounds(flag); 
		self.oFlags.set(flag);
	}
	
	constexpr void ClearFlag(this auto&& self, T flag) noexcept { 
		self.oFlags.reset(std::to_underlying(flag)); 
	}
	
	[[nodiscard]] constexpr size_t GetAllFlagNum(this auto const& self) noexcept { 
		return self.oFlags.to_ullong(); 
	}
	
	[[nodiscard]] constexpr size_t GetAllFlagCount(this auto const& self) noexcept { 
		return self.oFlags.count(); 
	}

private:
	static_assert(MaxValue <= std::numeric_limits<size_t>::max(), 
                 "T::Max exceeds std::bitset size limit");
	std::bitset<MaxValue> oFlags{};
};
