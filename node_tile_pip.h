#pragma once

#include <vector>
#include <bit>
#include <cstdint>
#include <set>
#include <span>
#include <cmath>
#include <execution>

class NodeTilePip {
public:
    uint64_t pip : 12;
    uint64_t tile_idx : 18;
    uint64_t node_idx : 25;

    uint64_t get_uint64_t() const noexcept {
        return std::bit_cast<uint64_t>(*this);
    }

    uint64_t get_key() const noexcept {
        return get_uint64_t() >> 30ull;
    }

//    friend bool operator== (const NodeTilePip s1, const NodeTilePip s2) { return s1.get_uint64_t() == s2.get_uint64_t(); }
//    friend auto operator<=>(const NodeTilePip s1, const NodeTilePip s2) { return s1.get_uint64_t() <=> s2.get_uint64_t(); }
};

static_assert(sizeof(NodeTilePip) == sizeof(uint64_t));
static_assert(std::is_trivial_v<NodeTilePip>);
static_assert(std::is_standard_layout_v<NodeTilePip>);
static_assert(alignof(NodeTilePip) == sizeof(uint64_t));

class Search_Node_Tile_Pip {
public:

	MemoryMappedFile mmf_node_tile_pip;
	std::span<NodeTilePip> node_tile_pip;

	Search_Node_Tile_Pip():
		mmf_node_tile_pip{ "sorted_node_tile_pip.bin" },
		node_tile_pip{ mmf_node_tile_pip.get_span<NodeTilePip>() }
	{
		puts(std::format("node_tile_pip: {}, {} bits", node_tile_pip.size(), ceil(log2(node_tile_pip.size()))).c_str());
	}

	inline constexpr static std::span<NodeTilePip> node_to_tile_pip(std::span<NodeTilePip> node_tile_pip, uint32_t node_idx) noexcept {
		return std::ranges::equal_range(node_tile_pip, NodeTilePip{ .node_idx{node_idx} }, [](NodeTilePip a, NodeTilePip b) { return a.node_idx < b.node_idx; });
	}

	inline constexpr std::span<NodeTilePip> node_to_tile_pip(uint32_t node_idx) const noexcept {
		return node_to_tile_pip(node_tile_pip, node_idx);
	}

	inline constexpr static std::span<NodeTilePip> get_node_tile_pips(std::span<NodeTilePip> node_tile_pip, uint32_t node_idx, uint32_t tile_idx, uint32_t pip_index) noexcept {
		return std::ranges::equal_range(node_tile_pip, NodeTilePip{ .pip{pip_index}, .tile_idx{tile_idx}, .node_idx{node_idx}}, [](NodeTilePip a, NodeTilePip b) { return a.get_uint64_t() < b.get_uint64_t(); });
	}

	static void make_node_tile_pip(Search_Wire_Tile_Node& search_wire_tile_node, ::DeviceResources::Device::Reader devRoot) {
		// SetNodeTilePip node_tile_pip;
		// 80000000 268435456
		puts("make_node_tile_pip start");
		MemoryMappedFile mmf_whole{ "sorted_node_tile_pip.bin", 0x100000000ull };
		std::atomic<size_t> storage_offset{};
		// alt_ret.reserve(250745691);
		                
		// node_tile_pip: 250745691, 28 bits

		{
			auto tiles{ devRoot.getTileList() };
			auto tile_types{ devRoot.getTileTypeList() };

			auto alt_ret{ mmf_whole.get_span<NodeTilePip>() };
			jthread_each(tiles, [&](uint64_t tile_idx, tile_reader &tile) {
				// WireTileNode key{ .tileStrIdx{tile.getName()} };
				auto tile_range{ search_wire_tile_node.tile_to_node(String_Index{._strIdx{tile.getName()}}) };
				// auto tile_range{ std::ranges::equal_range(wire_tile_node, key, [](WireTileNode a, WireTileNode b) {return a.tileStrIdx < b.tileStrIdx; }) };

				auto tile_type{ tile_types[tile.getType()] };
				auto tile_type_wire_strs{ tile_type.getWires() };
				auto tileIndex{ Tile_Index::make(tile) };

				each(tile_type.getPips(), [&](uint64_t pip_idx, pip_reader &pip) {
					auto wire0{ pip.getWire0() };
					auto wire1{ pip.getWire1() };
					auto directional{ pip.getDirectional() };
					auto isConventional{ pip.isConventional() };
					if (!isConventional) return;

					Wire_Info wi0{ ._wire_strIdx{._strIdx{tile_type_wire_strs[wire0]}}, ._tile_strIdx{._strIdx{tile.getName()}} };
					Wire_Info wi1{ ._wire_strIdx{._strIdx{tile_type_wire_strs[wire1]}}, ._tile_strIdx{._strIdx{tile.getName()}} };

					auto node0_idx{ Search_Wire_Tile_Node::wire_tile_to_node(tile_range, wi0.get_tile_strIdx(), wi0.get_wire_strIdx()) };
					auto node1_idx{ Search_Wire_Tile_Node::wire_tile_to_node(tile_range, wi1.get_tile_strIdx(), wi1.get_wire_strIdx()) };

					if (node0_idx != UINT32_MAX) {
						// node_tile_pip.insert({ .pip{pip_idx}, .tile_idx{static_cast<uint32_t>(tileIndex._value)}, .node_idx{node0_idx} });
						alt_ret[storage_offset.fetch_add(1)] = {.pip{pip_idx}, .tile_idx{static_cast<uint32_t>(tileIndex._value)}, .node_idx{node0_idx}};
					}

					if (node1_idx != UINT32_MAX) {
						// node_tile_pip.insert({ .pip{pip_idx}, .tile_idx{static_cast<uint32_t>(tileIndex._value)}, .node_idx{node1_idx} });
						alt_ret[storage_offset.fetch_add(1)] = {.pip{pip_idx}, .tile_idx{static_cast<uint32_t>(tileIndex._value)}, .node_idx{node1_idx}};
					}
				});
			});
		}

		auto mmf{ mmf_whole.shrink(storage_offset * sizeof(NodeTilePip)) };
		auto ret{ mmf.get_span<NodeTilePip>() };
		puts("make_node_tile_pip sort");

		std::sort(std::execution::par_unseq, ret.begin(), ret.end(), [](NodeTilePip a, NodeTilePip b) { return a.get_uint64_t() < b.get_uint64_t(); });
		puts("make_node_tile_pip finish");
	}

	void test(Search_Wire_Tile_Node& search_wire_tile_node, ::DeviceResources::Device::Reader devRoot) const {
		puts("Search_Node_Tile_Pip::test start");

		{
			auto tiles{ devRoot.getTileList() };
			auto tile_types{ devRoot.getTileTypeList() };

			each(tiles, [&](uint64_t tile_idx, tile_reader &tile) {
				auto tile_range{ search_wire_tile_node.tile_to_node(String_Index{._strIdx{tile.getName()}}) };

				auto tile_type{ tile_types[tile.getType()] };
				auto tile_type_wire_strs{ tile_type.getWires() };
				auto tileIndex{ Tile_Index::make(tile) };

				each(tile_type.getPips(), [&](uint64_t pip_idx, pip_reader &pip) {
					auto wire0{ pip.getWire0() };
					auto wire1{ pip.getWire1() };
					auto directional{ pip.getDirectional() };
					auto isConventional{ pip.isConventional() };
					if (!isConventional) return;

					Wire_Info wi0{ ._wire_strIdx{._strIdx{tile_type_wire_strs[wire0]}}, ._tile_strIdx{._strIdx{tile.getName()}} };
					Wire_Info wi1{ ._wire_strIdx{._strIdx{tile_type_wire_strs[wire1]}}, ._tile_strIdx{._strIdx{tile.getName()}} };

					auto node0_idx{ Search_Wire_Tile_Node::wire_tile_to_node(tile_range, wi0.get_tile_strIdx(), wi0.get_wire_strIdx()) };
					auto node1_idx{ Search_Wire_Tile_Node::wire_tile_to_node(tile_range, wi1.get_tile_strIdx(), wi1.get_wire_strIdx()) };

					if (node0_idx != UINT32_MAX) {
						auto found_ntps{ get_node_tile_pips(node_tile_pip, node0_idx, tileIndex._value, static_cast<uint32_t>(pip_idx)) };
						if (found_ntps.size() != 1) {
							abort();
						}
					}

					if (node1_idx != UINT32_MAX) {
						auto found_ntps{ get_node_tile_pips(node_tile_pip, node1_idx, tileIndex._value, static_cast<uint32_t>(pip_idx)) };
						if (found_ntps.size() != 1) {
							abort();
						}
					}
					});
				});
		}

		puts("Search_Node_Tile_Pip::test finish");
	}
};