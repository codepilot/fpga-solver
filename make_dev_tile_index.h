#pragma once

#include "lib_dev_flat.h"

template<typename T, T _size>
class limited_index {
private:
	inline static constexpr T size{ _size };
	T index{ UINT32_MAX };
public:
	inline constexpr limited_index() noexcept = default;
	inline constexpr limited_index(T _index) : index{ _index } { };
	inline constexpr static T pass_thru(T index) noexcept { if (index < size) return index; std::unreachable(); }
	inline constexpr operator T() const noexcept { return pass_thru(index); }
	inline constexpr bool is_valid() const noexcept { return index < size; }
	inline constexpr limited_index& operator++() { index = pass_thru(index) + 1; return *this; }
	inline constexpr limited_index& operator--() { index = pass_thru(index) + 1; return *this; }
};

namespace xcvu3p {
	using str_index = limited_index<uint32_t, ::xcvu3p::str_count>;
	using siteType_index = limited_index<uint32_t, ::xcvu3p::siteType_count>;
	using tileType_index = limited_index<uint32_t, ::xcvu3p::tileType_count>;
	using tile_index = limited_index<uint32_t, ::xcvu3p::tile_count>;
	using wire_index = limited_index<uint32_t, ::xcvu3p::wire_count>;
	using node_index = limited_index<uint32_t, ::xcvu3p::node_count>;
	using exceptionMap_index = limited_index<uint32_t, ::xcvu3p::exceptionMap_count>;
	using cellBelMap_index = limited_index<uint32_t, ::xcvu3p::cellBelMap_count>;
	using cellInversion_index = limited_index<uint32_t, ::xcvu3p::cellInversion_count>;
	using package_index = limited_index<uint32_t, ::xcvu3p::package_count>;
	using wireType_index = limited_index<uint32_t, ::xcvu3p::wireType_count>;
	using pipTiming_index = limited_index<uint32_t, ::xcvu3p::pipTiming_count>;
	using nodeTiming_index = limited_index<uint32_t, ::xcvu3p::nodeTiming_count>;
	constexpr auto each_tile{ each<tile_index, tile_list_reader, tile_reader> };
};

struct site_info {
	uint32_t tile_index: 24;
	uint32_t site_index: 8;//also site_type
};

