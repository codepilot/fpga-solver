#pragma once

#include "lib_dev_flat.h"
#include "lib_inverse_wires.h"

struct tile_idx_tile_wire_idx {
  uint32_t tile_idx, tile_wire_idx;
};

class Wire_Idx_to_Tile_Idx_Tile_Wire_Idx : public std::span<tile_idx_tile_wire_idx> {
public:
    static MemoryMappedFile make(std::string fn_bin) {
        {
            MemoryMappedFile mmf{ fn_bin, sizeof(std::array<tile_idx_tile_wire_idx, xcvu3p::wire_count>) };
            auto s_wire_idx_to_tile_idx_tile_wire_idx{ mmf.get_span<tile_idx_tile_wire_idx, xcvu3p::wire_count>() };
            std::ranges::fill(s_wire_idx_to_tile_idx_tile_wire_idx, tile_idx_tile_wire_idx{ .tile_idx{UINT32_MAX}, .tile_wire_idx{UINT32_MAX} });

            jthread_each<uint32_t>(xcvu3p::tiles, [&](uint32_t tile_idx, tile_reader tile) {
                auto tile_str_idx{ tile.getName() };
                auto tile_type{ xcvu3p::tileTypes[tile.getType()] };
                auto tile_wires{ tile_type.getWires() };
                auto tile_inverse_wires{ xcvu3p::inverse_wires.tile_range(tile_str_idx) };
                each<uint32_t>(tile_wires, [&](uint32_t tile_wire_idx, uint32_t wire_str_idx) {
                    auto v_wire_idx{ tile_inverse_wires.at(tile_str_idx, wire_str_idx) };
                    if (v_wire_idx.size() != 1) abort();
                    auto wire_idx{ v_wire_idx.front() };
                    s_wire_idx_to_tile_idx_tile_wire_idx[wire_idx] = tile_idx_tile_wire_idx{ .tile_idx{tile_idx}, .tile_wire_idx{tile_wire_idx} };
                    });
                });
        }
        return MemoryMappedFile{ fn_bin };
    }

};

// static inline const std::vector<tile_idx_tile_wire_idx> wire_idx_to_tile_idx_tile_wire_idx = make_wire_idx_to_tile_idx_tile_wire_idx();
