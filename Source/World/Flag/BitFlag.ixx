module;

export module BitFlag;

import std.compat;

template <typename T>
concept HasMaxField = requires {
    { T::Max };
	requires static_cast<std::underlying_type_t<T>>(T::Max) > 0;
};

export template<typename T>
requires HasMaxField<T>
class BitFlag
{
public:
	using NumType = std::underlying_type_t<T>;
	// using MaxTypeNum = std::numeric_limits<NumType>::max();

	constexpr void CheckBounds(NumType flag) const {
		if (flag >= static_cast<NumType>(T::Max)) {
			throw std::out_of_range("Flag value out of bounds");
		}
    }

	bool HasFlag(T flag) { return oFlags.test(static_cast<NumType>(flag)); }
	void SetFlag(T flag) { oFlags.set(static_cast<NumType>(flag)); }
	void SetFlag(NumType flag) { CheckBounds(flag); oFlags.set(flag);}
	void ClearFlag(T flag) { oFlags.reset(static_cast<NumType>(flag)); }
	uint64_t GetAllFlagNum() { return oFlags.to_ullong(); }
	uint32_t GetAllFlagCount() { return static_cast<uint32_t>(oFlags.count()); }
private:

	static_assert(static_cast<std::size_t>(T::Max) <= std::numeric_limits<std::size_t>::max(), 
                 "T::Max exceeds std::bitset size limit");
	std::bitset<static_cast<std::size_t>(T::Max)> oFlags;
};
