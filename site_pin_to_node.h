#pragma once

#include <algorithm>
#include <execution>
#include <span>
#include <vector>
#include "MemoryMappedFile.h"
#include "interchange_types.h"
#include "each.h"
#include <iostream>
#include "inverse_wires.h"
#include "Timer.h"
#include "InterchangeGZ.h"

class Site_Pin_to_Node: public std::span<const uint64_t> {
public:
	inline static constexpr uint64_t str_count{ 467843ull };
//  inline static constexpr uint64_t wire_count{ 83282368ull };
	inline static constexpr uint64_t node_count{ 28226432ull };
	inline static constexpr uint64_t site_pins_count{ 7926592ull };
	inline static constexpr uint64_t str_m_node_count{ str_count * node_count };

	inline constexpr Site_Pin_to_Node site_range(uint64_t site_strIdx) const {
		auto found{ std::ranges::equal_range(*this, site_strIdx * str_m_node_count, [&](uint64_t a, uint64_t b) { return (a / str_m_node_count) < (b / str_m_node_count); }) };
		if (found.empty()) {
			abort();
		}
		return Site_Pin_to_Node{ std::span<const uint64_t>(found) };
	}

	inline constexpr Site_Pin_to_Node wire_range(uint64_t site_strIdx, uint64_t pin_strIdx) const {
		auto found{ std::ranges::equal_range(*this, (site_strIdx * str_count + pin_strIdx) * node_count, [&](uint64_t a, uint64_t b) { return (a / node_count) < (b / node_count); }) };
//		if (found.empty()) {
//			abort();
//		}
		return Site_Pin_to_Node{ std::span<const uint64_t>(found) };
	}

	inline constexpr std::vector<uint32_t> at(uint64_t site_strIdx, uint64_t pin_strIdx) const {
		std::span<const uint64_t> found{ wire_range(site_strIdx, pin_strIdx) };
//		if (found.empty()) {
//			abort();
//		}
		std::vector<uint32_t> ret;
		ret.reserve(found.size());
		for (auto&& found_item : found) {
			ret.emplace_back(found_item % node_count);
		}
		return ret;
	}

	static uint64_t count_site_pins(const tile_list_reader tiles, const tile_type_list_reader tile_types) {
		std::atomic<uint64_t> ret{};
		jthread_each(tiles, [&](uint64_t tile_index, tile_reader & tile) {
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

	static std::vector<uint32_t> make_wire_idx_to_node_idx(wire_list_reader wires, node_list_reader nodes) {
		std::vector<uint32_t> wire_idx_to_node_idx(static_cast<size_t>(wires.size()), UINT32_MAX);
		jthread_each(nodes, [&](uint64_t node_idx, node_reader node) {
			for (auto wire_idx : node.getWires()) {
				wire_idx_to_node_idx[wire_idx] = static_cast<uint32_t>(node_idx);
			}
			});

		return wire_idx_to_node_idx;
	}

	inline static MemoryMappedFile make(device_reader devRoot, Inverse_Wires inverse_wires) {
		const node_list_reader nodes{ devRoot.getNodes() };
		const wire_list_reader wires{ devRoot.getWires() };
		const string_list_reader strList{ devRoot.getStrList() };
		const tile_list_reader tiles{ devRoot.getTileList() };
		const tile_type_list_reader tile_types{ devRoot.getTileTypeList() };
		const site_type_list_reader site_types{ devRoot.getSiteTypeList() };
		// const uint64_t site_pins_count{ count_site_pins(tiles, tile_types) };

		const auto wire_idx_to_node_idx{ TimerVal(make_wire_idx_to_node_idx(wires, nodes)) };

#if 1
		{
			MemoryMappedFile mmf{ "site_pin_to_node.bin", site_pins_count * sizeof(uint64_t) };
			auto site_pin_to_node{ mmf.get_span<uint64_t>() }; //wire str, tile str, wire idx log2(467843 * 467843 * 83282368) < 64
			std::atomic<uint64_t> site_pin_offset{};
			puts("site_pin_to_node gather");

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
					each(siteType.getPins(), [&](uint64_t pin_index, ::DeviceResources::Device::SitePin::Reader& pin) {
						auto pin_name{ pin.getName() };
						auto wire_str_idx{ tile_wires[pin_index] };
						auto wire_idx{ inverse_wires.at(tile_str_idx, wire_str_idx) };
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
						auto node_idx{ wire_idx_to_node_idx[wire_idx.front()] };
						if (node_idx == UINT32_MAX) return;
						site_pin_to_node[site_pin_offset++] = (site_name * str_count + pin_name) * node_count + node_idx;
					});
				}
			}





			puts("site_pin_to_node sort");
			std::sort(std::execution::par_unseq, site_pin_to_node.begin(), site_pin_to_node.end(), [](uint64_t a, uint64_t b) { return a < b; });
			puts("site_pin_to_node sorted");
		}
#endif
		MemoryMappedFile mmf_v_site_pin_to_node{ "site_pin_to_node.bin" };
#if 1
		const Site_Pin_to_Node site_pin_to_node{ mmf_v_site_pin_to_node.get_span<uint64_t>() };

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
				Site_Pin_to_Node pin_to_node{ site_pin_to_node.site_range(site_name) };
				each(site_pins, [&](uint64_t pin_index, ::DeviceResources::Device::SitePin::Reader& pin) {
					auto pin_name{ pin.getName() };
					auto wire_str_idx{ tile_wires[pin_index] };
					auto wire_idx{ inverse_wires.at(tile_str_idx, wire_str_idx) };
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
					auto expexted_node_idx{ wire_idx_to_node_idx[wire_idx.front()] };
					auto node_idx{ pin_to_node.at(site_name, pin_name) };
					if (expexted_node_idx == UINT32_MAX) {
						if (!node_idx.empty()) {
							abort();
						}
						return;
					}
					if (node_idx.empty()) {
						std::cout << std::format("site: {}, pin: {}, tile: {}, wire: {}\n",
							strList[site_name].cStr(),
							strList[pin_name].cStr(),
							strList[tile_str_idx].cStr(),
							strList[wire_str_idx].cStr()
						);
						abort();
						return;
					}
					if (node_idx.size() != 1) abort();
					if (node_idx.front() != expexted_node_idx) abort();
				});
			}
		}


#endif
		return mmf_v_site_pin_to_node;
	}
};