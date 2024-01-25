#pragma once

#include "PIP_Index.h"

#include "each.h"

#include "MemoryMappedFile.h"
#include <algorithm>
#include <cmath>
#include <execution>

class WireTileNode {
public:
    uint64_t leaf : 1;
    uint64_t node_branch : 25;
    uint64_t wireStrIdx : 19;
    uint64_t tileStrIdx : 19;

    uint64_t get_uint64_t() const noexcept {
        return std::bit_cast<uint64_t>(*this);
    }

    uint64_t get_key() const noexcept {
        return get_uint64_t() >> 26ull;
    }

//    friend bool operator== (const WireTileNode s1, const WireTileNode s2) { return s1.get_key() == s2.get_key(); }
//    friend auto operator<=>(const WireTileNode s1, const WireTileNode s2) { return s1.get_key() <=> s2.get_key(); }
};

// using SetWireTileNode = std::set<WireTileNode, decltype([](WireTileNode a, WireTileNode b){ return a < b; })>;

using WireTileNode8 = std::array<WireTileNode, 8>;
static_assert(sizeof(WireTileNode) == sizeof(uint64_t));
static_assert(std::is_trivial_v<WireTileNode>);
static_assert(std::is_standard_layout_v<WireTileNode>);
static_assert(alignof(WireTileNode) == sizeof(uint64_t));

class Search_Wire_Tile_Node {
public:

	MemoryMappedFile mmf_wire_tile_node;
	std::span<WireTileNode> wire_tile_node;

	Search_Wire_Tile_Node():
		mmf_wire_tile_node{ "sorted_wire_tile_node.bin" },
		wire_tile_node{ mmf_wire_tile_node.get_span<WireTileNode>() }
	{
		puts(std::format("wire_tile_node: {}, {} bits", wire_tile_node.size(), ceil(log2(wire_tile_node.size()))).c_str());
	}

	static std::span<WireTileNode> tile_to_node(std::span<WireTileNode> wire_tile_node, String_Index tileStrIdx) {
		auto key{ WireTileNode {.tileStrIdx{tileStrIdx._strIdx}} };
		return std::ranges::equal_range(wire_tile_node, key, [](WireTileNode a, WireTileNode b) { return a.tileStrIdx < b.tileStrIdx; });
	}

	std::span<WireTileNode> tile_to_node(String_Index tileStrIdx) const {
		return tile_to_node(wire_tile_node, tileStrIdx);
	}

	static uint32_t wire_tile_to_node(std::span<WireTileNode> wire_tile_node, String_Index tileStrIdx, String_Index wireStrIdx) {
		auto key{ WireTileNode {.wireStrIdx{wireStrIdx._strIdx},.tileStrIdx{tileStrIdx._strIdx}} };
		for (auto&& entry : std::ranges::equal_range(wire_tile_node, key, [](WireTileNode a, WireTileNode b) { return a.get_key() < b.get_key(); })) {
			return entry.node_branch;
		}
		return UINT32_MAX;
	}

	uint32_t wire_tile_to_node(String_Index tileStrIdx, String_Index wireStrIdx) const {
		return wire_tile_to_node(wire_tile_node, tileStrIdx, wireStrIdx);
	}

	static void make_wire_tile_node(::DeviceResources::Device::Reader devRoot) {
		auto nodes{ devRoot.getNodes() };
		auto wires{ devRoot.getWires() };
		puts("make_wire_tile_node() start");

		MemoryMappedFile mmf_whole{ "sorted_wire_tile_node.bin", wires.size() * sizeof(WireTileNode)};
		std::atomic<size_t> storage_offset{};

		{
			auto wire_tile_node_whole{ mmf_whole.get_span<WireTileNode>() };
			puts(std::format("wire_tile_node_whole: {}, {} bits", wire_tile_node_whole.size(), ceil(log2(wire_tile_node_whole.size()))).c_str());

			jthread_each(nodes, [&](uint64_t node_idx, node_reader &node) {
				auto node_wires{ node.getWires() };
				auto so_n{ storage_offset.fetch_add(node_wires.size()) };
				for (auto&& wire_idx : node_wires) {
					auto wire{ wires[wire_idx] };
					wire_tile_node_whole[so_n++] = {
						.leaf{},
						.node_branch{node_idx},
						.wireStrIdx{wire.getWire()},
						.tileStrIdx{wire.getTile()},
					};
				}
			});
		}

		auto mmf{mmf_whole.shrink(storage_offset * sizeof(WireTileNode))};
		auto wire_tile_node{ mmf.get_span<WireTileNode>() };
		puts(std::format("wire_tile_node: {}, {} bits", wire_tile_node.size(), ceil(log2(wire_tile_node.size()))).c_str());

		puts("make_wire_tile_node() sort");
		std::sort(std::execution::par_unseq, wire_tile_node.begin(), wire_tile_node.end(), [](WireTileNode a, WireTileNode b) { return a.get_uint64_t() < b.get_uint64_t(); });
		puts("make_wire_tile_node() finish");
	}

	void test(::DeviceResources::Device::Reader devRoot) const {
		auto nodes{ devRoot.getNodes() };
		auto wires{ devRoot.getWires() };
		puts("Search_Wire_Tile_Node::test() start");

		jthread_each(nodes, [&](uint64_t node_idx, node_reader &node) {
			for (auto&& wire_idx : node.getWires()) {
				auto wire{ wires[wire_idx] };

				auto found_node_idx{ wire_tile_to_node({wire.getTile()}, {wire.getWire()})};
				if (node_idx != found_node_idx) {
					abort();
				}
			}
		});

		puts("Search_Wire_Tile_Node::test() finish");
	}


};