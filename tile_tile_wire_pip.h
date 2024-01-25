#pragma once

#include "interchange_types.h"
#include "each.h"
#include "MMF_Dense_Sets.h"

class TileTileWirePip {
public:
    uint64_t pip_offset : 12;
    uint64_t wire_offset : 15;
    uint64_t tile_destination : 18;
    uint64_t tile_origin : 18;
    static TileTileWirePip make(Tile_Index tile_origin, Tile_Index tile_destination, uint16_t wire_offset, uint16_t pip_offset) {
        return {
            .pip_offset{pip_offset},
            .wire_offset{wire_offset},
            .tile_destination{static_cast<uint32_t>(tile_destination._value)},
            .tile_origin{static_cast<uint32_t>(tile_origin._value)},
        };
    }
};

class TilePip {
public:
    uint32_t pip_offset : 12;
    uint32_t tile_destination : 18;
    static TilePip make(Tile_Index tile_destination, uint16_t pip_offset) {
        return {
            .pip_offset{pip_offset},
            .tile_destination{static_cast<uint32_t>(tile_destination._value)},
        };
    }
    uint32_t get_uint32_t() const {
        return std::bit_cast<uint32_t>(*this);
    }
};

class Search_Tile_Tile_Wire_Pip {
public:
    MMF_Dense_Sets<TilePip> search_tile_tile_pip{ "search_tile_tile_pip.bin" };

    static void make(::DeviceResources::Device::Reader devRoot, std::span<uint32_t> dev_tile_strIndex_to_tile, Search_Wire_Tile_Node &search_wire_tile_node) {
        puts("Search_Tile_Tile_Wire_Pip::make start");
        auto nodes{ devRoot.getNodes() };
        auto tiles{ devRoot.getTileList() };
        auto tile_types{ devRoot.getTileTypeList() };
        auto wires{ devRoot.getWires() };
        // std::vector<TileTileWirePip> ttwp;
        std::vector<std::vector<TilePip>> tp;
        int32_t dx_max{ INT32_MIN };
        int32_t dy_max{ INT32_MIN };
        int32_t dx_min{ INT32_MAX };
        int32_t dy_min{ INT32_MAX };
        // ttwp.reserve(983379274ull);

        tp.resize(208370ull);

        jthread_each(tiles, [&](uint64_t tile_origin_idx, tile_reader &tile_origin) {
            // if (!(tile_origin_idx % 1000ull)) puts(std::format("{} of {}", tile_origin_idx, tiles.size()).c_str());
            auto tile_origin_strIdx{ tile_origin.getName() };
            auto tile_type_origin{ tile_types[tile_origin.getType()] };
            auto tile_index_origin{ Tile_Index::make(tile_origin) };
            auto tile_type_wire_strIdxs{ tile_type_origin.getWires() };
            auto tile_origin_pips{ tile_type_origin.getPips() };
            decltype(auto) tpi{ tp.at(tile_index_origin._value) };
            tpi.reserve(tile_origin_pips.size());
            each(tile_origin_pips, [&](uint64_t pip_idx, pip_reader &pip) {
                if (pip.isPseudoCells()) return;
                {
                    auto wire0_strIdx{ tile_type_wire_strIdxs[pip.getWire0()] };
                    auto node0_idx{ search_wire_tile_node.wire_tile_to_node(String_Index{._strIdx{tile_origin_strIdx} }, String_Index{._strIdx{wire0_strIdx} }) };
                    if (node0_idx != UINT32_MAX) {
                        auto node0{ nodes[node0_idx] };
                        for (auto&& node0_wire_idx : node0.getWires()) {
                            auto node0_wire{ wires[node0_wire_idx] };
                            auto node0_tile{ tiles[dev_tile_strIndex_to_tile[node0_wire.getTile()]] };
                            auto node0_tile_index{ Tile_Index::make(node0_tile) };
                            auto dx{ node0_tile_index.get_col() - tile_index_origin.get_col() };
                            auto dy{ node0_tile_index.get_row() - tile_index_origin.get_row() };
                            if (dx < dx_min) dx_min = dx;
                            if (dy < dy_min) dy_min = dy;
                            if (dx > dx_max) dx_max = dx;
                            if (dy > dy_max) dy_max = dy;
                            tpi.emplace_back(TilePip::make(node0_tile_index, static_cast<uint16_t>(pip_idx)));
                        }
                    }
                }
                if (!pip.getDirectional()) {
                    auto wire1_strIdx{ tile_type_wire_strIdxs[pip.getWire1()] };
                    auto node1_idx{ search_wire_tile_node.wire_tile_to_node(String_Index{._strIdx{tile_origin_strIdx} }, String_Index{._strIdx{wire1_strIdx} }) };
                    if (node1_idx != UINT32_MAX) {
                        auto node1{ nodes[node1_idx] };
                        for (auto&& node1_wire_idx : node1.getWires()) {
                            auto node1_wire{ wires[node1_wire_idx] };
                            auto node1_tile{ tiles[dev_tile_strIndex_to_tile[node1_wire.getTile()]] };
                            auto node1_tile_index{ Tile_Index::make(node1_tile) };
                            auto dx{ node1_tile_index.get_col() - tile_index_origin.get_col() };
                            auto dy{ node1_tile_index.get_row() - tile_index_origin.get_row() };
                            if (dx < dx_min) dx_min = dx;
                            if (dy < dy_min) dy_min = dy;
                            if (dx > dx_max) dx_max = dx;
                            if (dy > dy_max) dy_max = dy;

                            tpi.emplace_back(TilePip::make(node1_tile_index, static_cast<uint16_t>(pip_idx)));
                        }
                    }
                }
            });
            std::ranges::sort(tpi, [](TilePip a, TilePip b) { return a.get_uint32_t() < b.get_uint32_t(); });
        });
#if 0
        for (auto&& node : nodes) {
            auto node_wires{ node.getWires() };
            for (auto&& wireA_idx : node_wires) {
                auto wireA{ wires[wireA_idx] };
                auto tile_origin{ tiles[dev_tile_strIndex_to_tile[wireA.getTile()]]};
                auto tile_type_origin{ tile_types[tile_origin.getType()] };

                for (auto&& wireB_idx : node_wires) {
                    auto wireB{ wires[wireB_idx] };
                    auto tile_destination{ tiles[dev_tile_strIndex_to_tile[wireB.getTile()]] };
                    // ttwp.emplace_back(TileTileWirePip::make(Tile_Index::make(tile_origin), Tile_Index::make(tile_destination), UINT16_MAX, UINT16_MAX));
                }
            }
        }
#endif
//        puts(std::format("ttwp: {}", ttwp.size()).c_str());
        puts(std::format("dx: {} to {}, dy: {} to {}", dx_min, dx_max, dy_min, dy_max).c_str());
        puts("Search_Tile_Tile_Wire_Pip::make finish");
        MMF_Dense_Sets<TilePip>::make("search_tile_tile_pip.bin", tp);
    }
    void test(::DeviceResources::Device::Reader devRoot, std::span<uint32_t> dev_tile_strIndex_to_tile) {
    
    }
};