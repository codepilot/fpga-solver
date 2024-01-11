#pragma once

#include <cstdint>
#include <span>
#include "PIP_Index.h"
#include "MMF_Dense_Sets.h"
#include <unordered_set>
#include <set>

class RenumberedWires {
public:
    const MMF_Dense_Sets<Wire_Info> alt_wires{ "wires2.bin" }; //tested
    const MMF_Dense_Sets<uint32_t> alt_nodes{ "nodes2.bin" }; //tested
    const ::MemoryMappedFile wire_to_node_mmf{ "wire_to_node2.bin" }; //tested
    std::span<const uint32_t> alt_wire_to_node{ wire_to_node_mmf.get_span<const uint32_t>() }; //tested

    const MMF_Dense_Sets<PIP_Index> alt_wire_to_pips{ "wire_to_pips2.bin" };
    const MMF_Dense_Sets<PIP_Index> alt_node_to_pips{ "node_to_pips2.bin" };

    const ::MemoryMappedFile pips_mmf{ "pips2.bin" };
    std::span<const PIP_Info> alt_pips{ pips_mmf.get_span<const PIP_Info>() };

    // MMF_Dense_Sets_u32 alt_site_pin_wires{ "site_pin_wires2.bin" };
    // MMF_Dense_Sets_u32 alt_site_pin_nodes{ "site_pin_nodes2.bin" };
    const MMF_Dense_Sets<Site_Pin_Info> alt_site_pins{ "site_pin2.bin" };

    inline constexpr static uint32_t get_pip_wire0(decltype(alt_pips) alt_pips, PIP_Index pip_idx) noexcept {
        return alt_pips[pip_idx.get_id()].get_wire0();
    }

    inline constexpr uint32_t get_pip_wire0(PIP_Index pip_idx) const noexcept {
        return alt_pips[pip_idx.get_id()].get_wire0();
    }

    inline constexpr static uint32_t get_pip_wire1(decltype(alt_pips) alt_pips, PIP_Index pip_idx) noexcept {
        return alt_pips[pip_idx.get_id()].get_wire1();
    }

    inline constexpr uint32_t get_pip_wire1(PIP_Index pip_idx) const noexcept {
        return get_pip_wire1(alt_pips, pip_idx);
    }

    inline constexpr bool get_pip_directional(PIP_Index pip_idx) const noexcept {
        return alt_pips[pip_idx.get_id()].is_directional();
    }

    inline constexpr static uint32_t get_pip_node0(decltype(alt_wire_to_node) alt_wire_to_node, decltype(alt_pips) alt_pips, PIP_Index pip_idx) noexcept {
        auto wire0{ get_pip_wire0(alt_pips, pip_idx) };
        return alt_wire_to_node[wire0];
    }

    inline constexpr uint32_t get_pip_node0(PIP_Index pip_idx) const noexcept {
        auto wire0{ get_pip_wire0(pip_idx) };
        return alt_wire_to_node[wire0];
    }

    inline constexpr static uint32_t get_pip_node1(decltype(alt_wire_to_node) alt_wire_to_node, decltype(alt_pips) alt_pips, PIP_Index pip_idx) noexcept {
        auto wire1{ get_pip_wire1(alt_pips, pip_idx) };
        return alt_wire_to_node[wire1];
    }

    inline constexpr uint32_t get_pip_node1(PIP_Index pip_idx) const noexcept {
        return get_pip_node1(alt_wire_to_node, alt_pips, pip_idx);
    }

    inline constexpr uint32_t get_pip_node_in(PIP_Index pip_idx) const noexcept {
        return pip_idx.is_pip_forward() ? get_pip_node0(pip_idx) : get_pip_node1(pip_idx);
    }

    inline constexpr static uint32_t get_pip_node_out(decltype(alt_wire_to_node) alt_wire_to_node, decltype(alt_pips) alt_pips, PIP_Index pip_idx) noexcept {
        return pip_idx.is_pip_forward() ? get_pip_node1(alt_wire_to_node, alt_pips, pip_idx) : get_pip_node0(alt_wire_to_node, alt_pips, pip_idx);
    }

    inline constexpr uint32_t get_pip_node_out(PIP_Index pip_idx) const noexcept {
        return get_pip_node_out(alt_wire_to_node, alt_pips, pip_idx);
    }

    inline constexpr String_Index get_pip_wire0_str(PIP_Index pip_idx) const noexcept {
        return alt_wires.body[get_pip_wire0(pip_idx)].get_wire_strIdx();
    }

    inline constexpr String_Index get_pip_wire1_str(PIP_Index pip_idx) const noexcept {
        return alt_wires.body[get_pip_wire1(pip_idx)].get_wire_strIdx();
    }

    inline constexpr String_Index get_pip_tile0_str(PIP_Index pip_idx) const noexcept {
        return alt_wires.body[get_pip_wire0(pip_idx)].get_tile_strIdx();
    }

    inline constexpr String_Index get_wire_str(uint32_t wire_idx) const noexcept {
        return alt_wires.body[wire_idx].get_wire_strIdx();
    }

    inline constexpr String_Index get_wire_tile_str(uint32_t wire_idx) const noexcept {
        return alt_wires.body[wire_idx].get_tile_strIdx();
    }

    inline static uint32_t find_wire(decltype(alt_wires) &alt_wires, String_Index tile_strIdx, String_Index wire_strIdx) {
        uint64_t key{String_Index::make_key(wire_strIdx, tile_strIdx)};
        auto h{ _mm_crc32_u64(0, key) }; // not constexpr
        auto mask{ (alt_wires.size() - 1ull) };
        auto ret_index{ h & mask };
        auto bucket{ alt_wires[ret_index] };
        for (auto&& q : bucket) {
            if (q.get_key() == key) {
                uint32_t offset{ static_cast<uint32_t>(&q - alt_wires.body.data()) };
                return offset;
            }
        }
        // abort();
        return UINT32_MAX;
    }

#if 0
    inline constexpr static uint64_t combine_u32_u32_to_u64(uint32_t a, uint32_t b) {
        uint64_t ret{static_cast<uint64_t>(a) | (static_cast<uint64_t>(b) << 32ull)};
        return ret;
    }

    inline constexpr static uint32_t extract_low_u32_from_u64(uint64_t k) {
        return static_cast<uint32_t>(k);
    }

    inline constexpr static uint32_t extract_high_u32_from_u64(uint64_t k) {
        return static_cast<uint32_t>(k >> 32ull);
    }
#endif

    inline static uint32_t hash_site_pin(uint64_t hash_size, uint64_t key) {
        auto h{ _mm_crc32_u64(0, key) }; // not constexpr
        auto mask{ hash_size - 1ull };
        auto ret_index{ h & mask };
        return static_cast<uint32_t>(ret_index);
    }

    inline static uint32_t find_site_pin_wire(decltype(alt_site_pins)& alt_site_pins, String_Index site_strIdx, String_Index site_pin_strIdx) {
        auto key{ Site_Pin_Info::make_key(site_strIdx, site_pin_strIdx) };
        auto ret_index{ hash_site_pin(alt_site_pins.size(), key)}; // not constexpr
        auto bucket{ alt_site_pins[ret_index] };
        for (auto&& q : bucket) {
            if (q.get_key() == key) {
                return q.get_wire_idx();
            }
        }
        // abort();
        return UINT32_MAX;
    }

    inline static uint32_t find_site_pin_node(decltype(alt_site_pins)& alt_site_pins, String_Index site_strIdx, String_Index site_pin_strIdx) {
        auto key{ Site_Pin_Info::make_key(site_strIdx, site_pin_strIdx) };
        auto ret_index{ hash_site_pin(alt_site_pins.size(), key) }; //not constexpr
        auto bucket{ alt_site_pins[ret_index] };
        for (auto&& q : bucket) {
            if (q.get_key() == key) {
                return q.get_node_idx();
            }
        }
        // abort();
        return UINT32_MAX;
    }

    inline uint32_t find_wire(String_Index tile_strIdx, String_Index wire_strIdx) const {
        return find_wire(alt_wires, tile_strIdx, wire_strIdx); //not constexpr
    }

    inline uint32_t find_site_pin_wire(String_Index site_strIdx, String_Index site_pin_strIdx) const {
        return find_site_pin_wire(alt_site_pins, site_strIdx, site_pin_strIdx); //not constexpr
    }

    inline uint32_t find_site_pin_node(String_Index site_strIdx, String_Index site_pin_strIdx) const {
        return find_site_pin_node(alt_site_pins, site_strIdx, site_pin_strIdx); //not constexpr
    }

    void test_wires(DeviceResources::Device::Reader dev) const {
        auto wires{dev.getWires()};
        uint32_t good_wires{};
        uint32_t bad_wires{};

        std::vector<bool> wires_in_nodes(wires.size());

        for (auto&& node : dev.getNodes()) {
            for (auto&& wire : node.getWires()) {
                wires_in_nodes[wire] = true;
            }
        }

        for(uint32_t wire_idx{}; wire_idx < wires.size(); wire_idx++) {

            if (wire_idx && !(wire_idx % 1000000)) {
                // std::print("wire_idx: {}M, good_wires: {}, bad_wires: {}\n", wire_idx / 1000000, good_wires, bad_wires);
            }
            if (!wires_in_nodes.at(wire_idx)) continue;

            auto wire{ wires[wire_idx] };
            String_Index wire_str{ wire.getWire() };
            String_Index wire_tile_str{ wire.getTile() };

            auto alt_wire_offset{ find_wire(wire_tile_str, wire_str) };
            if (alt_wire_offset == UINT32_MAX) {
                bad_wires++;
                // std::print("wire_idx:{} not found\n", wire_idx);
                continue;
            }
            auto wire_found{ alt_wires.body[alt_wire_offset] };
            auto wire_found_wire_str{ wire_found.get_wire_strIdx() };
            auto wire_found_tile_str{ wire_found.get_wire_strIdx() };

            if (wire_found_wire_str != wire_str) {
                // std::print("wire_idx:{} alt_wire_offset:{} wire_found_wire_str:{}:{} != wire_str:{}:{}\n", wire_idx, alt_wire_offset, wire_found_wire_str, dev.getStrList()[wire_found_wire_str].cStr(), wire_str, dev.getStrList()[wire_str].cStr());
                // abort();
                bad_wires++;
                continue;
            }
            if (wire_found_tile_str != wire_tile_str) {
                // std::print("wire_idx:{} alt_wire_offset:{} wire_found_tile_str:{}:{} != wire_tile_str:{}:{}\n", wire_idx, alt_wire_offset, wire_found_tile_str, dev.getStrList()[wire_found_tile_str].cStr(), wire_tile_str, dev.getStrList()[wire_tile_str].cStr());
                // abort();
                bad_wires++;
                continue;
            }
            good_wires++;
        }
        // std::print("good_wires: {}, bad_wires: {}\n", good_wires, bad_wires);
    }

    void test_nodes(DeviceResources::Device::Reader dev) const {
        auto wires{ dev.getWires() };
        auto nodes{ dev.getNodes() };
        for (uint32_t nodeIdx{}; nodeIdx < nodes.size(); nodeIdx++) {
            if (nodeIdx && !(nodeIdx % 1000000)) {
                puts(std::format("nodeIdx: {}M", nodeIdx / 1000000).c_str());
            }

            auto node{ nodes[nodeIdx] };
            auto node_wires{ node.getWires() };
            auto alt_node{ alt_nodes[nodeIdx] };

            if (alt_node.size() != node_wires.size()) {
                puts("alt_node.size() != node_wires.size()");
                abort();
            }

            for (uint32_t node_wire_idx{}; node_wire_idx < node_wires.size(); node_wire_idx++) {
                auto wire_idx{ node_wires[node_wire_idx] };
                auto wire{ wires[wire_idx] };
                auto alt_wire_offset{ alt_node[node_wire_idx] };
                auto found_nodeIdx{ alt_wire_to_node[alt_wire_offset] };

                if (found_nodeIdx != nodeIdx) {
                    puts(std::format("nodeIdx: {}, found_nodeIdx: {}, node_wire_idx: {}, wire_idx: {}, alt_wire_offset: {}",
                        nodeIdx, found_nodeIdx, node_wire_idx, wire_idx, alt_wire_offset).c_str());

                    abort();
                }

                auto wire_found{ alt_wires.body[alt_wire_offset] };
                auto wire_found_wire_str{ wire_found.get_wire_strIdx() };
                auto wire_found_tile_str{ wire_found.get_tile_strIdx() };

                String_Index wire_str{ wire.getWire() };
                String_Index wire_tile_str{ wire.getTile() };
                if (wire_found_wire_str != wire_str) {
                    // std::print("nodeIdx:{} node_wire_idx:{} alt_wire_offset:{} wire_found_wire_str:{}:{} != wire_str:{}:{}\n", nodeIdx, node_wire_idx, alt_wire_offset, wire_found_wire_str, dev.getStrList()[wire_found_wire_str].cStr(), wire_str, dev.getStrList()[wire_str].cStr());
                    puts("wire_found_wire_str != wire_str");
                    abort();
                }
                if (wire_found_tile_str != wire_tile_str) {
                    // std::print("nodeIdx:{} node_wire_idx:{} alt_wire_offset:{} wire_found_tile_str:{}:{} != wire_tile_str:{}:{}\n", nodeIdx, node_wire_idx, alt_wire_offset, wire_found_tile_str, dev.getStrList()[wire_found_tile_str].cStr(), wire_tile_str, dev.getStrList()[wire_tile_str].cStr());
                    puts("wire_found_tile_str != wire_tile_str");
                    abort();
                }
            }
        }
    }

    void test_site_pin_wires(DeviceResources::Device::Reader dev) const {
        puts("test_site_pin_wires start");

        auto tiles{ dev.getTileList() };
        auto tileTypes{ dev.getTileTypeList() };
        auto siteTypes{ dev.getSiteTypeList() };
        auto wires{ dev.getWires() };
        auto nodes{ dev.getNodes() };

        size_t hash_size{ 1ull << (64ull - _lzcnt_u64(wires.size())) };
        size_t hash_mask{ hash_size - 1ull };
        puts(std::format("hash_size: {}, hash_mask: 0x{:x}", hash_size, hash_mask).c_str());

        size_t good_wires{};
        size_t good_nodes{};
        size_t bad_wires{};
        size_t bad_nodes{};

        for (auto&& tile : tiles) {
            String_Index tile_strIdx{ tile.getName() };
            auto tileType{ tileTypes[tile.getType()] };
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
                    uint64_t site_pin{ String_Index::make_key(site_strIdx, site_pin_strIdx) };
                    //ULARGE_INTEGER tile_wire{ .u{.LowPart{wire_strIdx}, .HighPart{tile_strIdx}} };
                    // auto wire_idx{ tile_strIdx_wire_strIdx_to_wire_idx.at(tile_wire.QuadPart) };
                    auto wire_idx{ find_wire(alt_wires, tile_strIdx, wire_strIdx) };
                    if (wire_idx == UINT32_MAX) {
                        continue;
                    }
                    auto node_idx{ alt_wire_to_node[wire_idx] };

                    auto key{ Site_Pin_Info::make_key(site_strIdx, site_pin_strIdx) };
                    if (site_pin != key) {
                        puts("site_pin != key");
                        abort();
                    }

                    auto hkey{ hash_site_pin(hash_size, key) };
                    auto h{ _mm_crc32_u64(0, site_pin) };
                    auto ret_index{ h & hash_mask };
                    if (hkey != ret_index) {
                        puts("hkey != ret_index");
                        abort();
                    }

                    auto found_wire_idx{ find_site_pin_wire(site_strIdx, site_pin_strIdx) };
                    auto found_node_idx{ find_site_pin_node(site_strIdx, site_pin_strIdx) };

                    if (wire_idx != found_wire_idx) {
                        // std::print("wire_idx:{} != found_wire_idx:{}\n", wire_idx, found_wire_idx);
                        // DebugBreak();
                        bad_wires++;

                    }
                    else {
                        good_wires++;
                    }

                    if (node_idx != found_node_idx) {
                        // std::print("node_idx:{} != found_node_idx:{}\n", node_idx, found_node_idx);
                        // DebugBreak();
                        bad_nodes++;
                    }
                    else {
                        good_nodes++;
                    }
                }
            }
        }
        puts(std::format("test_site_pin_wires wire {}:{}, node {}:{} finish", good_wires, bad_wires, good_nodes, bad_nodes).c_str());

    }

    static std::vector<std::vector<uint64_t>> make_wires(DeviceResources::Device::Reader dev) {

        puts("make_wires start");
        auto wires{ dev.getWires() };
        auto nodes{ dev.getNodes() };

        std::vector<std::vector<uint64_t>> node_renumber(nodes.size());

#if 0
        std::unordered_map<uint64_t, uint32_t> tile_strIdx_wire_strIdx_to_wire_idx;
        tile_strIdx_wire_strIdx_to_wire_idx.reserve(wires.size());
        for (uint32_t wireIdx{}; wireIdx < wires.size(); wireIdx++) {
            auto wire{ wires[wireIdx] };
            ULARGE_INTEGER key{ .u{.LowPart{wire.getWire()}, .HighPart{wire.getTile()}} };
            tile_strIdx_wire_strIdx_to_wire_idx.insert({ key.QuadPart, wireIdx });
        }
        puts("make tile_strIdx_wire_strIdx_to_wire_idx finish\n");
        return tile_strIdx_wire_strIdx_to_wire_idx;
#else
        size_t hash_size{ 1ull << (64ull - _lzcnt_u64(wires.size())) };
        size_t hash_mask{ hash_size - 1ull };
        puts(std::format("hash_size: {}, hash_mask: 0x{:x}", hash_size, hash_mask).c_str());

        std::vector<std::vector<Wire_Info>> ret(hash_size);
        // std::vector<std::vector<uint32_t>> wire_renumber(hash_size);

        for (uint32_t nodeIdx{}; nodeIdx < nodes.size(); nodeIdx++) {
            if (nodeIdx && !(nodeIdx % 1000000)) {
                puts(std::format("nodeIdx: {}M", nodeIdx / 1000000).c_str());
            }
            std::vector<uint64_t>& node_renumber_n{ node_renumber.at(nodeIdx) };
            auto node_wires{ nodes[nodeIdx].getWires() };
            node_renumber_n.reserve(node_wires.size());
            for (uint32_t node_wireIdx{}; node_wireIdx < node_wires.size(); node_wireIdx++) {
                auto wireIdx{ node_wires[node_wireIdx] };
                auto wire{ wires[wireIdx] };

                Wire_Info key{ ._wire_strIdx{ String_Index{ wire.getWire() }}, ._tile_strIdx{ String_Index{ wire.getTile() }} };
                auto h{ _mm_crc32_u64(0, key.get_key()) };
                auto ret_index{ h & hash_mask };
                std::vector<Wire_Info>& ret_at{ ret.at(ret_index) };
                uint64_t node_key{ std::bit_cast<uint64_t>(std::array<uint32_t, 2>{static_cast<uint32_t>(ret_index), static_cast<uint32_t>(ret_at.size())}) };
                ret_at.emplace_back(key);
                node_renumber_n.emplace_back(node_key);
            }
        }

#if 0
        for (uint32_t wireIdx{}; wireIdx < wires.size(); wireIdx++) {
            if (!(wireIdx % 1000000)) std::print("wireIdx: {}M\n", wireIdx / 1000000);
            auto wire{ wires[wireIdx] };
            ULARGE_INTEGER key{ .u{.LowPart{wire.getWire()}, .HighPart{wire.getTile()}} };
            auto h{ _mm_crc32_u64(0, key.QuadPart) };
            ret.at(h & hash_mask).emplace_back(key.QuadPart);
            wire_renumber.at(h & hash_mask).emplace_back(wireIdx);
        }
#endif
        puts("make_wires finish");

        // MMF_Dense_Sets_u32::make("wire_renumber.bin", wire_renumber);
        MMF_Dense_Sets<Wire_Info>::make("wires2.bin", ret);
        return node_renumber;
    }

    static void make_node_to_wires(DeviceResources::Device::Reader dev) {

        auto node_renumber{ make_wires(dev) };

        puts("make_node_to_wires start\n");

        auto nodes{ dev.getNodes() };
        auto wires{ dev.getWires() };

        auto alt_wires{ MMF_Dense_Sets<Wire_Info>{"wires2.bin"} };

        std::vector<std::vector<uint32_t>> node_final(nodes.size());
        for (uint32_t nodeIdx{}; nodeIdx < nodes.size(); nodeIdx++) {
            if (!(nodeIdx % 1000000)) puts(std::format("nodeIdx: {}M", nodeIdx / 1000000).c_str());
            auto node_wires{ node_renumber.at(nodeIdx) };
            decltype(node_final.at(nodeIdx)) node_final_n{ node_final.at(nodeIdx) };
            node_final_n.resize(node_wires.size());

            for (uint32_t node_wireIdx{}; node_wireIdx < node_wires.size(); node_wireIdx++) {
                auto node_wire_key{ node_wires[node_wireIdx] };
                uint64_t node_key{ node_wire_key };
                auto node_key_parts{ std::bit_cast<std::array<uint32_t, 2>>(node_key) };
                //.u{ .LowPart{static_cast<uint32_t>(ret_index)}, .HighPart{static_cast<uint32_t>(ret_at.size())} }
                auto ret_index{ node_key_parts[0]};
                auto ret_at_size{ node_key_parts[1]};
                auto offset{ alt_wires.get_offset(ret_index) + ret_at_size };
                auto wire_found{ alt_wires.body[offset] };

                auto wire_found_wire_str{ wire_found.get_wire_strIdx() };
                auto wire_found_tile_str{ wire_found.get_tile_strIdx() };

                auto wire{ wires[nodes[nodeIdx].getWires()[node_wireIdx]] };
                String_Index wire_str{ wire.getWire() };
                String_Index wire_tile_str{ wire.getTile() };
                if (wire_found_wire_str != wire_str) {
                    puts(std::format("wire_found_wire_str:{}:{} != wire_str:{}:{}",
                        wire_found_wire_str._strIdx, wire_found_wire_str.get_string_view(dev.getStrList()),
                        wire_str._strIdx, wire_str.get_string_view(dev.getStrList())).c_str());
                    abort();
                }
                if (wire_found_tile_str != wire_tile_str) {
                    puts(std::format("wire_found_tile_str:{}:{} != wire_tile_str:{}:{}",
                        wire_found_tile_str._strIdx, wire_found_tile_str.get_string_view(dev.getStrList()),
                        wire_tile_str._strIdx, wire_tile_str.get_string_view(dev.getStrList())).c_str());
                    abort();
                }
                node_final_n[node_wireIdx] = static_cast<uint32_t>(offset);
            }
        }

        MMF_Dense_Sets<uint32_t>::make("nodes2.bin", node_final);

        puts("make_node_to_wires finish\n");
#endif
    }

    static void make_wire_to_node() {
        puts("make_wire_to_node start");

        MMF_Dense_Sets<Wire_Info> alt_wires{ "wires2.bin" }; //tested
        MMF_Dense_Sets<uint32_t> alt_nodes{ "nodes2.bin" }; //tested


        MemoryMappedFile wire_to_node_mmf{ "wire_to_node2.bin", alt_wires.body.size() * sizeof(uint32_t) };
        auto alt_wire_to_node{ wire_to_node_mmf.get_span<uint32_t>() };
        for (uint32_t node_idx{}; node_idx < alt_nodes.size(); node_idx++) {
            if (node_idx && !(node_idx % 1000000)) {
                puts(std::format("node_idx: {}M", node_idx / 1000000).c_str());
            }
            for (auto wire_idx : alt_nodes[node_idx]) {
                alt_wire_to_node[wire_idx] = node_idx;
            }
        }

        puts("make_wire_to_node finish");
    }

    static void make_wire_to_pips(DeviceResources::Device::Reader dev) {
        puts("start make_wire_to_pips\n");

        MMF_Dense_Sets<Wire_Info> alt_wires{ "wires2.bin" }; //tested

        auto alt_wires_body{ alt_wires.body };
        auto tiles{ dev.getTileList() };
        auto tileTypes{ dev.getTileTypeList() };

        std::vector<std::vector<PIP_Index>> wire_to_pips{ static_cast<size_t>(alt_wires_body.size()) };

        MemoryMappedFile pips_mmf{ "pips2.bin", 4294967296ull };

        auto pips_span{ pips_mmf.get_span<uint64_t>() };

        // std::vector<std::vector<uint64_t>> pips{ 1ull };


        uint32_t pips_size{};
        {
            uint32_t pip_counter{};

            for (auto&& tile : tiles) {
                String_Index tile_strIdx{ tile.getName() };
                auto tileType{ tileTypes[tile.getType()] };
                auto tileType_wires{ tileType.getWires() };
                for (auto&& pip : tileType.getPips()) {
                    if (pip.isPseudoCells()) continue;

                    String_Index wire0_strIdx{ tileType_wires[pip.getWire0()] };
                    String_Index wire1_strIdx{ tileType_wires[pip.getWire1()] };


                    // ULARGE_INTEGER key0{ .u{.LowPart{wire0_strIdx}, .HighPart{tile_strIdx}} };
                    // ULARGE_INTEGER key1{ .u{.LowPart{wire1_strIdx}, .HighPart{tile_strIdx}} };
                    // auto wire0_idx{ tile_strIdx_wire_strIdx_to_wire_idx.at(key0.QuadPart) };
                    // auto wire1_idx{ tile_strIdx_wire_strIdx_to_wire_idx.at(key1.QuadPart) };
                    auto wire0_idx{ find_wire(alt_wires, tile_strIdx, wire0_strIdx) };
                    if (wire0_idx == UINT32_MAX) continue;

                    auto wire1_idx{ find_wire(alt_wires, tile_strIdx, wire1_strIdx) };
                    if (wire1_idx == UINT32_MAX) continue;

                    // ULARGE_INTEGER pipInfo{ .u{.LowPart{wire0_strIdx}, .HighPart{tile_strIdx}} };

                    if (pip_counter && !(pip_counter % 1000000)) {
                        puts(std::format("pip_counter: {}M", pip_counter / 1000000).c_str());
                    }

                    auto pip_idx_forward{ PIP_Index::make(pip_counter, true) };
                    auto pip_idx_reverse{ PIP_Index::make(pip_counter, false) };

                    uint64_t pip_info{
                        (static_cast<uint64_t>(wire0_idx) & 0xfffffffull) |
                        ((static_cast<uint64_t>(wire1_idx) & 0xfffffffull) << 32ull) |
                        (static_cast<uint64_t>(pip.getDirectional()) << 63ull) //is directional
                    };

                    pips_span[pip_counter] = pip_info;
                    pip_counter++;

                    if (pip.getDirectional()) {
                        wire_to_pips[wire0_idx].push_back(pip_idx_forward);
                    }
                    else {
                        wire_to_pips[wire0_idx].push_back(pip_idx_forward);
                        wire_to_pips[wire1_idx].push_back(pip_idx_reverse);
                    }
                }
            }
            pips_size = static_cast<uint64_t>(pip_counter) * sizeof(uint64_t);
        }

        auto pips_mmf_shrunk{ pips_mmf.shrink(pips_size) };

        puts("finish make_wire_to_pips\n");

        MMF_Dense_Sets<PIP_Index>::make("wire_to_pips2.bin", wire_to_pips);

    }

    static void make_node_to_pips(DeviceResources::Device::Reader dev) {
        puts("start make_node_to_pips");

        ::MemoryMappedFile pips_mmf{ "pips2.bin" };
        std::span<PIP_Info> alt_pips{ pips_mmf.get_span<PIP_Info>() };

        MMF_Dense_Sets<Wire_Info> alt_wires{ "wires2.bin" }; //tested
        MMF_Dense_Sets<uint32_t> alt_nodes{ "nodes2.bin" }; //tested

        MemoryMappedFile wire_to_node_mmf{ "wire_to_node2.bin" };
        auto alt_wire_to_node{ wire_to_node_mmf.get_span<uint32_t>() };

        std::vector<std::vector<PIP_Index>> node_to_pips{ alt_nodes.size() };

        for (uint32_t pip_counter{}; pip_counter < alt_pips.size(); pip_counter++) {
            if (pip_counter && !(pip_counter % 1000000)) {
                puts(std::format("pip_counter: {}M", pip_counter / 1000000).c_str());
            }

            auto pip_info{ alt_pips[pip_counter] };
            auto wire0{ pip_info.get_wire0() };
            auto wire1{ pip_info.get_wire1() };
            bool directional{ pip_info.is_directional() };

            auto node0_idx{ alt_wire_to_node[wire0] };
            auto node1_idx{ alt_wire_to_node[wire1] };

            PIP_Index pip_idx_forward{ PIP_Index::make(pip_counter, true) };
            PIP_Index pip_idx_reverse{ PIP_Index::make(pip_counter, false) };

            node_to_pips[node0_idx].emplace_back(pip_idx_forward);
            if (!directional) {
                node_to_pips[node1_idx].emplace_back(pip_idx_reverse);
            }
        }

        puts("finish make_node_to_pips");

        MMF_Dense_Sets<PIP_Index>::make("node_to_pips2.bin", node_to_pips);
    }

    static void make_site_pin_wires(DeviceResources::Device::Reader dev) {
        puts("make_site_pin_wires start");

        MMF_Dense_Sets<Wire_Info> alt_wires{ "wires2.bin" }; //tested

        ::MemoryMappedFile wire_to_node_mmf{ "wire_to_node2.bin" }; //tested
        std::span<uint32_t> alt_wire_to_node{ wire_to_node_mmf.get_span<uint32_t>() }; //tested

        auto tiles{ dev.getTileList() };
        auto tileTypes{ dev.getTileTypeList() };
        auto siteTypes{ dev.getSiteTypeList() };
        auto wires{ dev.getWires() };
        auto nodes{ dev.getNodes() };

        size_t hash_size{ 1ull << (64ull - _lzcnt_u64(wires.size())) };
        size_t hash_mask{ hash_size - 1ull };
        puts(std::format("hash_size: {}, hash_mask: 0x{:x}", hash_size, hash_mask).c_str());

        std::vector<std::vector<Site_Pin_Info>> site_pin_wires_kv(hash_size);

        for (auto&& tile : tiles) {
            String_Index tile_strIdx{ tile.getName() };
            auto tileType{ tileTypes[tile.getType()] };
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
                    uint64_t site_pin_key{ String_Index::make_key(site_strIdx, site_pin_strIdx) };
                    //ULARGE_INTEGER tile_wire{ .u{.LowPart{wire_strIdx}, .HighPart{tile_strIdx}} };
                    // auto wire_idx{ tile_strIdx_wire_strIdx_to_wire_idx.at(tile_wire.QuadPart) };
                    auto wire_idx{ find_wire(alt_wires, tile_strIdx, wire_strIdx) };
                    if (wire_idx == UINT32_MAX) {
                        continue;
                    }
                    auto node_idx{ alt_wire_to_node[wire_idx] };

                    auto h{ _mm_crc32_u64(0, site_pin_key) };
                    auto ret_index{ h & hash_mask };
                    site_pin_wires_kv.at(ret_index).emplace_back(Site_Pin_Info{
                        ._site{site_strIdx},
                        ._site_pin{site_pin_strIdx},
                        ._wire{wire_idx},
                        ._node{node_idx},
                    });
                }
            }
        }

        puts("make_site_pin_wires finish");
        MMF_Dense_Sets<Site_Pin_Info>::make("site_pin2.bin", site_pin_wires_kv);
    }

    static void test(DeviceResources::Device::Reader dev) {
        RenumberedWires wr;
        wr.test_nodes(dev);
        wr.test_wires(dev);
        wr.test_site_pin_wires(dev);
    }

    static void make_pip_group_r(
        DeviceResources::Device::Reader dev,
        std::vector<PIP_Index> &ret,
        PIP_Index pip_idx,
        decltype(alt_wires)& alt_wires,
        decltype(alt_nodes)& alt_nodes,
        decltype(alt_wire_to_node) alt_wire_to_node,
        decltype(alt_node_to_pips)& alt_node_to_pips,
        decltype(alt_pips) alt_pips
    ) {
    }

    static std::vector<PIP_Index> make_pip_group(
        DeviceResources::Device::Reader dev,
        PIP_Index pip_idx,
        decltype(alt_wires) &alt_wires,
        decltype(alt_nodes) &alt_nodes,
        decltype(alt_wire_to_node) alt_wire_to_node,
        decltype(alt_node_to_pips) &alt_node_to_pips,
        decltype(alt_pips) alt_pips
    ) {
        std::vector<PIP_Index> ret;
        get_pip_node_out(alt_wire_to_node, alt_pips, pip_idx);
        make_pip_group_r(dev, ret, pip_idx, alt_wires, alt_nodes, alt_wire_to_node, alt_node_to_pips, alt_pips);
        return ret;
    }

    static void make_pip_groups(DeviceResources::Device::Reader dev) {
        const MMF_Dense_Sets<Wire_Info> alt_wires{ "wires2.bin" }; //tested
        const MMF_Dense_Sets<uint32_t> alt_nodes{ "nodes2.bin" }; //tested
        const ::MemoryMappedFile wire_to_node_mmf{ "wire_to_node2.bin" }; //tested
        std::span<const uint32_t> alt_wire_to_node{ wire_to_node_mmf.get_span<const uint32_t>() }; //tested

        const MMF_Dense_Sets<PIP_Index> alt_wire_to_pips{ "wire_to_pips2.bin" };
        const MMF_Dense_Sets<PIP_Index> alt_node_to_pips{ "node_to_pips2.bin" };

        const ::MemoryMappedFile pips_mmf{ "pips2.bin" };
        std::span<const PIP_Info> alt_pips{ pips_mmf.get_span<const PIP_Info>() };


        std::vector<std::vector<std::vector<PIP_Index>>> pip_groups(alt_nodes.size()); // node_idx to vector of vectors of pip_indexes
        for (uint32_t node_idx{}; node_idx < alt_nodes.size(); node_idx++) {
            decltype(pip_groups.at(node_idx)) nv{ pip_groups.at(node_idx) };
            auto node_pips{ alt_node_to_pips[node_idx] };
            nv.reserve(node_pips.size());
            for (auto&& pip : node_pips) {
                nv.emplace_back(make_pip_group(dev, pip, alt_wires, alt_nodes, alt_wire_to_node, alt_node_to_pips, alt_pips));
            }
        }
        puts("");
        // for()
    }

    static std::vector<std::set<uint32_t>> propagate_tile_wires(DeviceResources::Device::Reader dev, DeviceResources::Device::TileType::Reader tileType) {
        std::string_view name{ dev.getStrList()[tileType.getName()].cStr()};
        auto wires{ tileType.getWires() };
        auto pips{ tileType.getPips() };
        auto siteTypes{ tileType.getSiteTypes() };
        std::vector<std::set<uint32_t>> wire_to_wires(static_cast<size_t>(wires.size()));

        std::set<uint32_t> tile_inputs;
        std::set<uint32_t> tile_outputs;

        for (auto pip: pips) {
            auto wire0{ pip.getWire0() };
            auto wire1{ pip.getWire1() };
            auto directional{ pip.getDirectional() };
            auto isConventional{ pip.isConventional() };
            if (!isConventional) continue;
            wire_to_wires.at(wire0).insert(wire1);
            tile_inputs.insert(wire0);
            tile_outputs.insert(wire1);

            if (directional) {
                // puts(std::format("  {}->{}", wire0, wire1).c_str());
            }
            else {
                // puts(std::format("  {}<>{}", wire0, wire1).c_str());
                wire_to_wires.at(wire1).insert(wire0);
                tile_inputs.insert(wire1);
                tile_outputs.insert(wire0);
            }
        }

        // puts(std::format("{} siteTypes:{} wires:{} in:{} out:{} pips:{}", name, siteTypes.size(), wires.size(), tile_inputs.size(), tile_outputs.size(), pips.size()).c_str());

        size_t total_added_wires{};
        for (;;) {
            // puts(std::format("total_added_wires: {}", total_added_wires).c_str());
            bool added_wires{};
            for (auto&& dst_wires : wire_to_wires) {
                for (auto&& dst_wire: dst_wires) {
                    auto size_before{ dst_wires.size() };
                    dst_wires.insert_range(wire_to_wires[dst_wire]);
                    auto size_after{ dst_wires.size() };
                    if (size_before != size_after) {
                        total_added_wires += size_after - size_before;
                        added_wires = true;
                        break;
                    }
                }
                // if (added_wires) break;
            }
            if (!added_wires) break;
        }

        for (uint32_t wire_idx{}; wire_idx < wires.size(); wire_idx++) {
            auto dst_wires{ wire_to_wires.at(wire_idx) };
            if (!dst_wires.size()) continue;
#if 0
            std::string wires_str{};
            for (auto&& dst_wire_idx : dst_wires) {
                wires_str += " " + std::to_string(dst_wire_idx);
            }
            puts(std::format("  {}->{}", wire_idx, wires_str).c_str());
#endif
             // puts(std::format("  {}.size:{}", wire_idx, dst_wires.size()).c_str());
        }
        // puts("");
        return wire_to_wires;
    }

    static void propagate_wires(DeviceResources::Device::Reader dev) {
        auto wires{ dev.getWires() };
        auto tiles{ dev.getTileList() };
        auto tileTypes{ dev.getTileTypeList() };

        std::vector<std::vector<std::set<uint32_t>>> tile_wire_to_wires(tileTypes.size());
        std::unordered_map<uint32_t, uint32_t> tile_str_idx_to_tile_idx(tiles.size());
        std::unordered_map<uint64_t, uint32_t> rev_wires(wires.size());
        std::vector<std::unordered_map<uint32_t, uint32_t>> tile_ws_to_wi(tiles.size());

        for (uint32_t tile_idx{}; tile_idx < tiles.size(); tile_idx++) {
            tile_str_idx_to_tile_idx[tiles[tile_idx].getName()] = tile_idx;
        }

        for (uint32_t wire_idx{}; wire_idx < wires.size(); wire_idx++) {
            auto wire{ wires[wire_idx] };
            auto tile_idx{ tile_str_idx_to_tile_idx.at(wire.getTile()) };
            tile_ws_to_wi.at(tile_idx).insert({wire.getWire(), wire_idx});
            rev_wires.insert({ std::bit_cast<uint64_t>(std::array<uint32_t, 2>{wire.getTile(), wire.getWire()}), wire_idx });
        }

        for (uint32_t tile_type_idx{}; tile_type_idx < tileTypes.size(); tile_type_idx++) {
            auto tileType{ tileTypes[tile_type_idx] };
            if (!tileType.getPips().size()) continue;
            tile_wire_to_wires.at(tile_type_idx) = propagate_tile_wires(dev, tileType);
        }

        for (auto&& tile : dev.getTileList()) {
            auto tileType_idx{ tile.getType() };
            auto tileType{ tileTypes[tileType_idx] };
            decltype(tile_wire_to_wires.at(tileType_idx)) ww{ tile_wire_to_wires.at(tileType_idx) };
        }

        DebugBreak();
    }

    static void make_pips(DeviceResources::Device::Reader dev) {
        make_node_to_wires(dev);
        make_wire_to_node();
        make_wire_to_pips(dev);
        make_node_to_pips(dev);
        make_site_pin_wires(dev);
        make_pip_groups(dev); // given a node_idx, get a pip group that leaves the tile
        test(dev);
    }

    static const RenumberedWires load() {
        return RenumberedWires{};
    }
};
