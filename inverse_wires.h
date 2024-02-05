#pragma once

#include <algorithm>
#include <execution>
#include <span>
#include <vector>
#include "MemoryMappedFile.h"
#include "interchange_types.h"
#include "each.h"
#include <iostream>



class Inverse_Wires: public std::span<const uint64_t> {
public:
	inline static constexpr uint64_t str_count{ 467843ull };
	inline static constexpr uint64_t wire_count{ 83282368ull };
	inline static constexpr uint64_t str_m_wire_count{ str_count * wire_count };

	inline constexpr Inverse_Wires tile_range(uint64_t tile_strIdx) const {
		Inverse_Wires ret{ std::ranges::equal_range(*this, tile_strIdx * str_m_wire_count, [&](uint64_t a, uint64_t b) { return (a / str_m_wire_count) < (b / str_m_wire_count); }) };
		return ret;
	}

	inline constexpr auto wire_range(uint64_t tile_strIdx, uint64_t wire_strIdx) const {
		auto ret{ std::ranges::equal_range(*this, (tile_strIdx * str_count + wire_strIdx) * wire_count, [&](uint64_t a, uint64_t b) { return (a / wire_count) < (b / wire_count); }) };
		return ret;
	}

	inline constexpr std::vector<uint32_t> at(uint64_t tile_strIdx, uint64_t wire_strIdx) const {
		std::span<const uint64_t> found{ wire_range(tile_strIdx, wire_strIdx) };
		if (found.empty()) {
			abort();
		}
		std::vector<uint32_t> ret;
		ret.reserve(found.size());
		for (auto&& found_item : found) {
			ret.emplace_back(found_item % wire_count);
		}
		return ret;
	}

	inline static MemoryMappedFile make(const std::string file_name, const wire_list_reader wires, const string_list_reader strList, const tile_list_reader tiles, const tile_type_list_reader tile_types) {
		{
			MemoryMappedFile mmf{file_name, wire_count * sizeof(uint64_t) };
			auto inverse_wires{ mmf.get_span<uint64_t>() }; //wire str, tile str, wire idx log2(467843 * 467843 * 83282368) < 64
			puts("inverse_wires gather");
			jthread_each(wires, [&](uint64_t wire_idx, wire_reader& wire) {
				uint64_t tile_strIdx{ wire.getTile() };
				uint64_t wire_strIdx{ wire.getWire() };
				inverse_wires[wire_idx] = (tile_strIdx * str_count + wire_strIdx) * wire_count + wire_idx;
			});
			puts("inverse_wires sort");
			std::sort(std::execution::par_unseq, inverse_wires.begin(), inverse_wires.end(), [](uint64_t a, uint64_t b) { return a < b; });
			puts("inverse_wires sorted");
		}
		MemoryMappedFile mmf_v_inverse_wires{ file_name };
		const Inverse_Wires inverse_wires{ mmf_v_inverse_wires.get_span<uint64_t>() };

		for (auto&& tile : tiles) {
			auto tile_str_idx{ tile.getName() };
			auto tileType{ tile_types[tile.getType()] };
			auto tileTypeSiteTypes{ tileType.getSiteTypes() };
			for (auto&& siteTypeInTile : tileTypeSiteTypes) {
				auto tile_wires{ siteTypeInTile.getPrimaryPinsToTileWires() };
				for (auto&& wire_str_idx : tile_wires) {
					auto wire_idx{ inverse_wires.at(tile_str_idx, wire_str_idx) };
					if (wire_idx.empty()) {
						std::cout << std::format("tile: {}, wire: {}\n",
							strList[tile_str_idx].cStr(),
							strList[wire_str_idx].cStr()
						);
						abort();
					}
				}
			}
		}
		return mmf_v_inverse_wires;
	}
};