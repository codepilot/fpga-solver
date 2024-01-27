#pragma once

#include <algorithm>
#include <execution>

class Inverse_Wires: public std::span<const uint64_t> {
public:
	inline static constexpr uint64_t str_count{ 467843ull };
	inline static constexpr uint64_t wire_count{ 83282368ull };
	inline static constexpr uint64_t str_m_wire_count{ str_count * wire_count };

	inline constexpr auto tile_range(uint64_t tile_strIdx) const {
		auto ret{ std::ranges::equal_range(*this, tile_strIdx * str_m_wire_count, [&](uint64_t a, uint64_t b) { return (a / str_m_wire_count) < (b / str_m_wire_count); }) };
		return ret;
	}

	inline constexpr auto wire_range(uint64_t tile_strIdx, uint64_t wire_strIdx) const {
		auto ret{ std::ranges::equal_range(*this, (tile_strIdx * str_count + wire_strIdx) * wire_count, [&](uint64_t a, uint64_t b) { return (a / wire_count) < (b / wire_count); }) };
		return ret;
	}

	inline constexpr uint32_t at(uint64_t tile_strIdx, uint64_t wire_strIdx) const {
		auto ret{ wire_range(tile_strIdx, wire_strIdx).front() };
		return ret % wire_count;
	}

	inline static MemoryMappedFile make(const wire_list_reader wires) {
		MemoryMappedFile mmf{ "Inverse_Wires.bin", wire_count * sizeof(uint64_t) };
		auto inverse_wires{ mmf.get_span<uint64_t>() }; //wire str, tile str, wire idx log2(467843 * 467843 * 83282368) < 64
		puts("inverse_wires gather");
		jthread_each(wires, [&](uint64_t wire_idx, wire_reader& wire) {
			uint64_t tile_strIdx{ wire.getTile() };
			uint64_t wire_strIdx{ wire.getWire() };
			inverse_wires[wire_idx] = (tile_strIdx * str_count + wire_strIdx) * wire_count + wire_idx;
			});
		puts("inverse_wires sort");
		std::sort(std::execution::par_unseq, inverse_wires.begin(), inverse_wires.end(), [](uint64_t a, uint64_t b) { return a < b; });
		return mmf;
	}
};