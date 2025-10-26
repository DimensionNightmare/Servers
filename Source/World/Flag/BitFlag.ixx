export module BitFlag;

import std.compat;

template <typename T>
concept HasMaxField = requires {
    { T::Max };
	requires std::underlying_type_t<T>(T::Max) > 0;
};

export template<typename T>
requires HasMaxField<T>
class BitFlag
{
public:
	using NumType = std::underlying_type_t<T>;
	// using MaxTypeNum = std::numeric_limits<NumType>::max();

	constexpr void CheckBounds(NumType flag) const {
		if (flag >= std::to_underlying(T::Max)) {
			throw std::out_of_range("Flag value out of bounds");
		}
    }

	bool HasFlag(T flag) { return oFlags.test(std::to_underlying(flag)); }
	void SetFlag(T flag) { oFlags.set(std::to_underlying(flag)); }
	void SetFlag(NumType flag) { CheckBounds(flag); oFlags.set(flag);}
	void ClearFlag(T flag) { oFlags.reset(std::to_underlying(flag)); }
	size_t GetAllFlagNum() { return oFlags.to_ullong(); }
	size_t GetAllFlagCount() { return oFlags.count(); }
private:

	static_assert(std::to_underlying(T::Max) <= std::numeric_limits<size_t>::max(), 
                 "T::Max exceeds std::bitset size limit");
	std::bitset<std::to_underlying(T::Max)> oFlags;
};
