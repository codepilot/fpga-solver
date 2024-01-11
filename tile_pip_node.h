#pragma once

#include "MemoryMappedFile.h"
#include <span>
#include <vector>
#include <algorithm>


#include "node_tile_pip.h"

class TilePipNode {
public:
    uint64_t node_idx : 25;
    uint64_t pip : 12;
    uint64_t tile_idx : 18;

    uint64_t get_uint64_t() const noexcept {
        return std::bit_cast<uint64_t>(*this);
    }

    uint64_t get_key() const noexcept {
        return get_uint64_t() >> 25ull;
    }

    // friend bool operator== (const TilePipNode s1, const TilePipNode s2) { return s1.get_uint64_t() == s2.get_uint64_t(); }
	// friend auto operator<=>(const TilePipNode s1, const TilePipNode s2) { return s1.get_uint64_t() <=> s2.get_uint64_t(); }
};


static_assert(sizeof(TilePipNode) == sizeof(uint64_t));
static_assert(std::is_trivial_v<TilePipNode>);
static_assert(std::is_standard_layout_v<TilePipNode>);
static_assert(alignof(TilePipNode) == sizeof(uint64_t));


class Search_Tile_Pip_Node {
public:

	MemoryMappedFile mmf_tile_pip_node;
	std::span<TilePipNode> tile_pip_node;

	Search_Tile_Pip_Node():
		mmf_tile_pip_node{ "sorted_tile_pip_node.bin" },
		tile_pip_node{ mmf_tile_pip_node.get_span<TilePipNode>() }
	{
		puts(std::format("tile_pip_node: {}, {} bits", tile_pip_node.size(), ceil(log2(tile_pip_node.size()))).c_str());
	}

	static void save_tile_pip_node(std::span<TilePipNode> tile_pip_node) {
		MemoryMappedFile mmf{ "sorted_tile_pip_node.bin", tile_pip_node.size_bytes() };
		memcpy(mmf.fp, tile_pip_node.data(), tile_pip_node.size_bytes());
	}

	static void save_tile_pip_node(std::span<NodeTilePip> node_tile_pip) {
		puts("save_tile_pip_node start");
		std::vector<TilePipNode> tile_pip_node;

		tile_pip_node.reserve(node_tile_pip.size());
		// convert each char to size_t
		std::ranges::transform(node_tile_pip, std::back_inserter(tile_pip_node),
			[](NodeTilePip ntp) -> TilePipNode { return TilePipNode{ .node_idx{ntp.node_idx}, .pip{ntp.pip}, .tile_idx{ntp.tile_idx} }; });

		puts("save_tile_pip_node sort");
		std::ranges::sort(tile_pip_node, [](TilePipNode a, TilePipNode b) { return a.get_uint64_t() < b.get_uint64_t();  });

		save_tile_pip_node(tile_pip_node);
		puts("save_tile_pip_node finish");
	}


};