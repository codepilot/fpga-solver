#pragma once
#include <cstdint>
#include <bit>
#include <set>
#include <algorithm>
#include "interchange_types.h"
#include <cmath>
#include "wire_tile_wire.h"

class SitePinWire {
public:
	/*
    uint64_t leaf : 1;
    uint64_t wire_branch : 25;
    uint64_t sitePinStrIdx : 19;
    uint64_t siteStrIdx : 19;
	*/

	uint64_t _value;

    uint64_t get_uint64_t() const noexcept {
        return _value;
    }

	inline constexpr uint64_t get_key() const noexcept {
		return _value / 83282368ull;
	}

	inline constexpr uint64_t get_siteStrIdx() const noexcept {
		return _value / (83282368ull * 467843ull);
	}

	inline constexpr uint64_t get_sitePinStrIdx() const noexcept {
		return (_value / 83282368ull) % 467843ull;
	}

	inline constexpr uint32_t get_wire_idx() const noexcept {
		return static_cast<uint32_t>(_value % 83282368ull);
	}

	inline constexpr static SitePinWire make(uint64_t siteStrIdx, uint64_t sitePinStrIdx, uint64_t wire_idx) noexcept {
		return { ._value{((siteStrIdx * (467843ull * 83282368ull)) + (sitePinStrIdx * 83282368ull) + wire_idx)} };
	}
};
static_assert(sizeof(SitePinWire) == sizeof(uint64_t));
static_assert(std::is_trivial_v<SitePinWire>);
static_assert(std::is_standard_layout_v<SitePinWire>);
static_assert(alignof(SitePinWire) == sizeof(uint64_t));

class Search_Site_Pin_Wire {
public:

    MemoryMappedFile mmf_site_pin_wires;
    std::span<SitePinWire> site_pin_wires;

    Search_Site_Pin_Wire(): mmf_site_pin_wires{ "sorted_site_pin_wires.bin" }, site_pin_wires{ mmf_site_pin_wires.get_span<SitePinWire>() } {
        puts(std::format("site_pin_wires: {}, {} bits", site_pin_wires.size(), ceil(log2(site_pin_wires.size()))).c_str());
    }

	static uint32_t site_pin_to_wire(std::span<SitePinWire> site_pin_wire, String_Index siteStrIdx, String_Index sitePinStrIdx) {
		auto key{ SitePinWire::make(siteStrIdx._strIdx, sitePinStrIdx._strIdx, 0) };
		auto found{ std::ranges::equal_range(site_pin_wire, key, [](SitePinWire a, SitePinWire b) { return a.get_key() < b.get_key(); }) };
		for (auto&& entry : found) {
			return entry.get_wire_idx();
		}
		return UINT32_MAX;
	}

	static std::vector<uint32_t> site_pins_to_wires(::capnp::List< ::capnp::Text, ::capnp::Kind::BLOB>::Reader physStrs, std::span<SitePinWire> site_pin_wires, std::vector<String_Index> &phys_stridx_to_dev_stridx, uint32_t net_strIdx, branch_list_reader site_pins) {
		std::vector<uint32_t> source_wires;
		source_wires.reserve(site_pins.size());

		for (auto&& site_pin : site_pins) {
			auto sp{ site_pin.getRouteSegment().getSitePin() };
			auto wire_idx{ site_pin_to_wire(site_pin_wires, {phys_stridx_to_dev_stridx.at(sp.getSite())}, {phys_stridx_to_dev_stridx.at(sp.getPin())}) };
			if (UINT32_MAX == wire_idx) {
				abort();
				continue;
			}
			source_wires.emplace_back(wire_idx);
		}

		return source_wires;
	}

	static std::vector<uint32_t> site_pins_to_wires(::capnp::List< ::capnp::Text, ::capnp::Kind::BLOB>::Reader physStrs, std::span<SitePinWire> site_pin_wires, std::vector<String_Index> &phys_stridx_to_dev_stridx, uint32_t net_strIdx, std::span<branch_reader> site_pins) {
		std::vector<uint32_t> source_wires;
		source_wires.reserve(site_pins.size());

		for (auto&& site_pin : site_pins) {
			auto sp{ site_pin.getRouteSegment().getSitePin() };
			auto wire_idx{ site_pin_to_wire(site_pin_wires, {phys_stridx_to_dev_stridx.at(sp.getSite())}, {phys_stridx_to_dev_stridx.at(sp.getPin())}) };
			if (UINT32_MAX == wire_idx) {
				abort();
				continue;
			}
			source_wires.emplace_back(wire_idx);
		}

		return source_wires;
	}

	static std::vector<uint32_t> source_site_pins_to_wires(::capnp::List< ::capnp::Text, ::capnp::Kind::BLOB>::Reader physStrs, std::span<SitePinWire> site_pin_wires, std::vector<String_Index>& phys_stridx_to_dev_stridx, uint32_t net_strIdx, branch_list_reader sources) {
		auto site_pins{ RouteStorage::branches_site_pins(physStrs, net_strIdx, sources) };
		return site_pins_to_wires(physStrs, site_pin_wires, phys_stridx_to_dev_stridx, net_strIdx, site_pins);
	}

	static void make_site_pin_wires(std::span<WireTileWire> wire_tile_wire, ::DeviceResources::Device::Reader devRoot) {

        // decltype(phys.root) physRoot{ phys.root };
        decltype(devRoot.getStrList()) devStrs{ devRoot.getStrList() };
        decltype(devRoot.getTileList()) tiles{ devRoot.getTileList() };
        decltype(devRoot.getTileTypeList()) tile_types{ devRoot.getTileTypeList() };
        decltype(devRoot.getSiteTypeList()) siteTypes{ devRoot.getSiteTypeList() };
        // decltype(physRoot.getStrList()) physStrs{ physRoot.getStrList() };
        decltype(devRoot.getWires()) wires{ devRoot.getWires() };
        decltype(devRoot.getWireTypes()) wireTypes{ devRoot.getWireTypes() };

		puts("make_site_pin_wires start");

		MemoryMappedFile mmf_whole{ "sorted_site_pin_wires.bin", wires.size() * sizeof(SitePinWire) };
		size_t storage_offset{};
		uint32_t missing_wires{};

		{
			auto site_pin_wire{ mmf_whole.get_span<SitePinWire>() };


			for (auto&& tile : tiles) {
				String_Index tile_strIdx{ tile.getName() };
				auto tileType{ tile_types[tile.getType()] };
				for (auto&& site : tile.getSites()) {
					String_Index site_strIdx{ site.getName() };
					auto site_type_idx{ site.getType() };
					auto site_tile_type{ tileType.getSiteTypes()[site_type_idx] };
					auto site_type{ siteTypes[site_tile_type.getPrimaryType()] };
					auto site_pins{ site_type.getPins() };
					auto site_tile_type_wires{ site_tile_type.getPrimaryPinsToTileWires() };
					for (uint32_t pin_idx{}; pin_idx < site_pins.size(); pin_idx++) {
						String_Index wire_strIdx{ site_tile_type_wires[pin_idx] };
						auto sitePin{ site_pins[pin_idx] };
						String_Index site_pin_strIdx{ sitePin.getName() };

						auto wire_idx{ Search_Wire_Tile_Wire::wire_tile_to_wire(wire_tile_wire, tile_strIdx, wire_strIdx) };
						if (wire_idx == UINT32_MAX) {
#if 0
							puts(std::format("site:{} pin:{} tile:{} wire:{} missing wire",
								devStrs[site.getName()].cStr(),
								devStrs[sitePin.getName()].cStr(),
								tile_strIdx.get_string_view(devStrs),
								wire_strIdx.get_string_view(devStrs)
							).c_str());
#endif
							missing_wires++;
							continue;
						}

						site_pin_wire[storage_offset++] = SitePinWire::make(site_strIdx._strIdx, site_pin_strIdx._strIdx, wire_idx);

					}
				}
			}
		}

		auto mmf{ mmf_whole.shrink(storage_offset * sizeof(SitePinWire)) };
		auto site_pin_wire{ mmf.get_span<SitePinWire>() };

		// puts(std::format("make_site_pin_wires finish missing_wires:{}", missing_wires).c_str());

		puts("make_site_pin_wires sort");
		std::ranges::sort(site_pin_wire, [](SitePinWire a, SitePinWire b) { return a.get_uint64_t() < b.get_uint64_t(); });
		puts("make_site_pin_wires finish");
	}


};