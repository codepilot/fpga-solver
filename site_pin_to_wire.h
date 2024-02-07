#pragma once

#include "lib_inverse_wires.h"

class Site_Pin_to_Wire: public std::span<const uint64_t> {
public:
	inline static constexpr uint64_t str_count{ 467843ull };
  inline static constexpr uint64_t wire_count{ 83282368ull };
	inline static constexpr uint64_t site_pins_count{ 7926592ull };
	inline static constexpr uint64_t str_m_wire_count{ str_count * wire_count };

	inline constexpr Site_Pin_to_Wire site_range(uint64_t site_strIdx) const {
		auto found{ std::ranges::equal_range(*this, site_strIdx * str_m_wire_count, [&](uint64_t a, uint64_t b) { return (a / str_m_wire_count) < (b / str_m_wire_count); }) };
		if (found.empty()) {
			abort();
		}
		return Site_Pin_to_Wire{ std::span<const uint64_t>(found) };
	}

	inline constexpr Site_Pin_to_Wire wire_range(uint64_t site_strIdx, uint64_t pin_strIdx) const {
		auto found{ std::ranges::equal_range(*this, (site_strIdx * str_count + pin_strIdx) * wire_count, [&](uint64_t a, uint64_t b) { return (a / wire_count) < (b / wire_count); }) };
//		if (found.empty()) {
//			abort();
//		}
		return Site_Pin_to_Wire{ std::span<const uint64_t>(found) };
	}

	inline constexpr std::vector<uint32_t> at(uint64_t site_strIdx, uint64_t pin_strIdx) const {
		std::span<const uint64_t> found{ wire_range(site_strIdx, pin_strIdx) };
//		if (found.empty()) {
//			abort();
//		}
		std::vector<uint32_t> ret;
		ret.reserve(found.size());
		for (auto&& found_item : found) {
			ret.emplace_back(static_cast<uint32_t>(found_item % wire_count));
		}
		return ret;
	}

	static uint64_t count_site_pins(const tile_list_reader tiles, const tile_type_list_reader tile_types) {
		std::atomic<uint64_t> ret{};
		jthread_each<uint32_t>(tiles, [&](uint32_t tile_index, tile_reader tile) {
			auto tile_str_idx{ tile.getName() };
			auto tileType{ tile_types[tile.getType()] };
			auto tileTypeSiteTypes{ tileType.getSiteTypes() };
			for (auto&& siteTypeInTile : tileTypeSiteTypes) {
				auto tile_wires{ siteTypeInTile.getPrimaryPinsToTileWires() };
				ret += tile_wires.size();
			}
		});
		return ret.load();
	}

	inline static MemoryMappedFile make(std::string fn_bin, device_reader devRoot) {
		const wire_list_reader wires{ devRoot.getWires() };
		const string_list_reader strList{ devRoot.getStrList() };
		const tile_list_reader tiles{ devRoot.getTileList() };
		const tile_type_list_reader tile_types{ devRoot.getTileTypeList() };
		const site_type_list_reader site_types{ devRoot.getSiteTypeList() };

#if 1
		{
			MemoryMappedFile mmf{ fn_bin, site_pins_count * sizeof(uint64_t) };
			auto site_pin_to_wire{ mmf.get_span<uint64_t>() }; //wire str, tile str, wire idx log2(467843 * 467843 * 83282368) < 64
			std::atomic<uint64_t> site_pin_offset{};
			puts("site_pin_to_wire gather");

			for (auto&& tile : tiles) {
				auto tile_str_idx{ tile.getName() };
				auto sites{ tile.getSites() };
				auto tileType{ tile_types[tile.getType()] };
				auto tileTypeSiteTypes{ tileType.getSiteTypes() };

				for (auto&& site : tile.getSites()) {
					auto site_name{ site.getName() };
					auto siteTypeInTile{ tileTypeSiteTypes[site.getType()] };
					auto siteType{ site_types[siteTypeInTile.getPrimaryType()] };
					auto tile_wires{ siteTypeInTile.getPrimaryPinsToTileWires() };
					auto pins{ siteType.getPins() };
					each<uint32_t>(pins, [&](uint32_t pin_index, ::DeviceResources::Device::SitePin::Reader pin) {
						auto pin_name{ pin.getName() };
						auto wire_str_idx{ tile_wires[pin_index] };
						auto wire_idx{ xcvu3p::inverse_wires.at(tile_str_idx, wire_str_idx) };
						if (wire_idx.empty()) {
							std::cout << std::format("site: {}, pin: {}, tile: {}, wire: {}\n",
								strList[site_name].cStr(),
								strList[pin_name].cStr(),
								strList[tile_str_idx].cStr(),
								strList[wire_str_idx].cStr()
							);
							abort();
							return;
						}
						site_pin_to_wire[site_pin_offset++] = (site_name * str_count + pin_name) * wire_count + static_cast<uint64_t>(wire_idx.front());
					});
				}
			}





			puts("site_pin_to_wire sort");
			std::sort(std::execution::par_unseq, site_pin_to_wire.begin(), site_pin_to_wire.end(), [](uint64_t a, uint64_t b) { return a < b; });
			puts("site_pin_to_wire sorted");
		}
#endif
		MemoryMappedFile mmf_v_site_pin_to_wire{ fn_bin };
#if 1
		const Site_Pin_to_Wire site_pin_to_wire{ mmf_v_site_pin_to_wire.get_span<uint64_t>() };

		for (auto&& tile : tiles) {
			auto tile_str_idx{ tile.getName() };
			auto sites{ tile.getSites() };
			auto tileType{ tile_types[tile.getType()] };
			auto tileTypeSiteTypes{ tileType.getSiteTypes() };

			for (auto&& site : tile.getSites()) {
				auto site_name{ site.getName() };
				auto siteTypeInTile{ tileTypeSiteTypes[site.getType()] };
				auto siteType{ site_types[siteTypeInTile.getPrimaryType()] };
				auto tile_wires{ siteTypeInTile.getPrimaryPinsToTileWires() };
				auto site_pins{ siteType.getPins() };
				if (!site_pins.size()) continue;
				Site_Pin_to_Wire pin_to_wire{ site_pin_to_wire.site_range(site_name) };
				each<uint32_t>(site_pins, [&](uint32_t pin_index, ::DeviceResources::Device::SitePin::Reader pin) {
					auto pin_name{ pin.getName() };
					auto wire_str_idx{ tile_wires[pin_index] };
					auto v_wire_idx{ xcvu3p::inverse_wires.at(tile_str_idx, wire_str_idx) };
					if (v_wire_idx.empty()) {
						std::cout << std::format("site: {}, pin: {}, tile: {}, wire: {}\n",
							strList[site_name].cStr(),
							strList[pin_name].cStr(),
							strList[tile_str_idx].cStr(),
							strList[wire_str_idx].cStr()
						);
						abort();
						return;
					}






					auto expexted_wire_idx{ v_wire_idx.front() };
					auto wire_idx{ pin_to_wire.at(site_name, pin_name) };
					if (expexted_wire_idx == UINT32_MAX) {
						if (!wire_idx.empty()) {
							abort();
						}
						return;
					}
					if (wire_idx.empty()) {
						std::cout << std::format("site: {}, pin: {}, tile: {}, wire: {}\n",
							strList[site_name].cStr(),
							strList[pin_name].cStr(),
							strList[tile_str_idx].cStr(),
							strList[wire_str_idx].cStr()
						);
						abort();
						return;
					}
					if (wire_idx.size() != 1) abort();
					if (wire_idx.front() != expexted_wire_idx) abort();

				});
			}
		}


#endif
		return mmf_v_site_pin_to_wire;
	}
};