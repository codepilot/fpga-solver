#pragma once
#include <cstdint>
#include <bit>
#include <set>
#include <algorithm>
#include "interchange_types.h"
#include <cmath>
#include "wire_tile_node.h"
#include <execution>

class SitePinNode {
public:
    uint64_t leaf : 1;
    uint64_t node_branch : 25;
    uint64_t sitePinStrIdx : 19;
    uint64_t siteStrIdx : 19;

    uint64_t get_uint64_t() const noexcept {
        return std::bit_cast<uint64_t>(*this);
    }

    uint64_t get_key() const noexcept {
        return get_uint64_t() >> 26ull;
    }

//    bool operator ==(const SitePinNode& b) const noexcept { return get_key() == b.get_key(); }
//    bool operator !=(const SitePinNode& b) const noexcept { return get_key() != b.get_key(); }
//    bool operator >(const SitePinNode& b) const noexcept { return get_key() > b.get_key(); }
//    bool operator <(const SitePinNode& b) const noexcept { return get_key() < b.get_key(); }

    // friend bool operator== (const SitePinNode s1, const SitePinNode s2) { return s1.get_key() == s2.get_key(); }
    // friend auto operator<=>(const SitePinNode s1, const SitePinNode s2) { return s1.get_key() <=> s2.get_key(); }

};
//using SetSitePinNode = std::set<SitePinNode, decltype([](SitePinNode a, SitePinNode b) { return a < b; })>;
using SitePinNodeGroup = std::array<SitePinNode, 8>;
static_assert(sizeof(SitePinNode) == sizeof(uint64_t));
static_assert(std::is_trivial_v<SitePinNode>);
static_assert(std::is_standard_layout_v<SitePinNode>);
static_assert(alignof(SitePinNode) == sizeof(uint64_t));

class Search_Site_Pin_Node {
public:

    MemoryMappedFile mmf_site_pin_nodes;
    std::span<SitePinNode> site_pin_nodes;

    Search_Site_Pin_Node(): mmf_site_pin_nodes{ "sorted_site_pin_nodes.bin" }, site_pin_nodes{ mmf_site_pin_nodes.get_span<SitePinNode>() } {
        puts(std::format("site_pin_nodes: {}, {} bits", site_pin_nodes.size(), ceil(log2(site_pin_nodes.size()))).c_str());
    }

#if 0
	static uint32_t site_pin_to_node(SetSitePinNode& site_pin_node, String_Index siteStrIdx, String_Index sitePinStrIdx) {
		auto key{ SitePinNode {.sitePinStrIdx{sitePinStrIdx._strIdx},.siteStrIdx{siteStrIdx._strIdx}} };
		auto found{ site_pin_node.equal_range(key) };

		for (auto entry{ found.first }; entry != found.second; entry++) {
			return entry->node_branch;
		}
		return UINT32_MAX;
	}
#endif

	static uint32_t site_pin_to_node(std::span<SitePinNode> site_pin_node, String_Index siteStrIdx, String_Index sitePinStrIdx) {
		auto key{ SitePinNode {.sitePinStrIdx{sitePinStrIdx._strIdx},.siteStrIdx{siteStrIdx._strIdx}} };
		for (auto&& entry : std::ranges::equal_range(site_pin_node, key, [](SitePinNode a, SitePinNode b) { return a.get_key() < b.get_key(); })) {
			return entry.node_branch;
		}
		return UINT32_MAX;
	}

	static std::vector<uint32_t> site_pins_to_nodes(::capnp::List< ::capnp::Text, ::capnp::Kind::BLOB>::Reader physStrs, std::span<SitePinNode> site_pin_nodes, std::vector<String_Index> &phys_stridx_to_dev_stridx, uint32_t net_strIdx, branch_list_reader site_pins) {
		std::vector<uint32_t> source_nodes;
		source_nodes.reserve(site_pins.size());

		for (auto&& site_pin : site_pins) {
			auto sp{ site_pin.getRouteSegment().getSitePin() };
			auto node_idx{ site_pin_to_node(site_pin_nodes, {phys_stridx_to_dev_stridx.at(sp.getSite())}, {phys_stridx_to_dev_stridx.at(sp.getPin())}) };
			if (UINT32_MAX == node_idx) {
				abort();
				continue;
			}
			source_nodes.emplace_back(node_idx);
		}

		return source_nodes;
	}

	static std::vector<uint32_t> site_pins_to_nodes(::capnp::List< ::capnp::Text, ::capnp::Kind::BLOB>::Reader physStrs, std::span<SitePinNode> site_pin_nodes, std::vector<String_Index>& phys_stridx_to_dev_stridx, uint32_t net_strIdx, std::span<branch_reader> site_pins) {
		std::vector<uint32_t> source_nodes;
		source_nodes.reserve(site_pins.size());

		for (auto&& site_pin : site_pins) {
			auto sp{ site_pin.getRouteSegment().getSitePin() };
			auto node_idx{ site_pin_to_node(site_pin_nodes, {phys_stridx_to_dev_stridx.at(sp.getSite())}, {phys_stridx_to_dev_stridx.at(sp.getPin())}) };
			if (UINT32_MAX == node_idx) {
				abort();
				continue;
			}
			source_nodes.emplace_back(node_idx);
		}

		return source_nodes;
	}

	static std::vector<uint32_t> source_site_pins_to_nodes(::capnp::List< ::capnp::Text, ::capnp::Kind::BLOB>::Reader physStrs, std::span<SitePinNode> site_pin_nodes, std::vector<String_Index>& phys_stridx_to_dev_stridx, uint32_t net_strIdx, branch_list_reader sources) {
		auto site_pins{ RouteStorage::branches_site_pins(physStrs, net_strIdx, sources) };
		return site_pins_to_nodes(physStrs, site_pin_nodes, phys_stridx_to_dev_stridx, net_strIdx, site_pins);
	}

	static void make_site_pin_nodes(std::span<WireTileNode> wire_tile_node, ::DeviceResources::Device::Reader devRoot) {

        // decltype(phys.root) physRoot{ phys.root };
        decltype(devRoot.getStrList()) devStrs{ devRoot.getStrList() };
        decltype(devRoot.getTileList()) tiles{ devRoot.getTileList() };
        decltype(devRoot.getTileTypeList()) tile_types{ devRoot.getTileTypeList() };
        decltype(devRoot.getSiteTypeList()) siteTypes{ devRoot.getSiteTypeList() };
        // decltype(physRoot.getStrList()) physStrs{ physRoot.getStrList() };
        decltype(devRoot.getWires()) wires{ devRoot.getWires() };
        decltype(devRoot.getWireTypes()) wireTypes{ devRoot.getWireTypes() };
        decltype(devRoot.getNodes()) nodes{ devRoot.getNodes() };

		puts("make_site_pin_nodes start");

		MemoryMappedFile mmf_whole{ "sorted_site_pin_nodes.bin", nodes.size() * sizeof(SitePinNode) };
		std::atomic<size_t> storage_offset{};
		uint32_t missing_nodes{};

		{
			auto site_pin_node{ mmf_whole.get_span<SitePinNode>() };


			jthread_each(tiles, [&](uint64_t tile_idx, tile_reader &tile) {
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

						auto node_idx{ Search_Wire_Tile_Node::wire_tile_to_node(wire_tile_node, tile_strIdx, wire_strIdx) };
						if (node_idx == UINT32_MAX) {
#if 0
							puts(std::format("site:{} pin:{} tile:{} wire:{} missing node",
								devStrs[site.getName()].cStr(),
								devStrs[sitePin.getName()].cStr(),
								tile_strIdx.get_string_view(devStrs),
								wire_strIdx.get_string_view(devStrs)
							).c_str());
#endif
							missing_nodes++;
							continue;
						}

						site_pin_node[storage_offset.fetch_add(1)] = {.node_branch{node_idx}, .sitePinStrIdx{site_pin_strIdx._strIdx}, .siteStrIdx{site_strIdx._strIdx}};

#if 0
						if (site_pin_to_node(site_pin_node, site_strIdx, site_pin_strIdx) != node_idx) {
							abort();
						}
#endif
					}
				}
			});
		}

		auto mmf{ mmf_whole.shrink(storage_offset * sizeof(SitePinNode)) };
		auto site_pin_node{ mmf.get_span<SitePinNode>() };

		// puts(std::format("make_site_pin_nodes finish missing_nodes:{}", missing_nodes).c_str());

		puts("make_site_pin_nodes sort");
		std::sort(std::execution::par_unseq, site_pin_node.begin(), site_pin_node.end(), [](SitePinNode a, SitePinNode b) { return a.get_uint64_t() < b.get_uint64_t(); });
		puts("make_site_pin_nodes finish");
	}


};