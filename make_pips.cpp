#include <capnp/serialize.h>
#include "DeviceResources.capnp.h"
#include "LogicalNetlist.capnp.h"
#include "PhysicalNetlist.capnp.h"

#include <cstdint>
#include <type_traits>
#include <immintrin.h>
#include <cstdint>
#include <array>
#include <span>
#include <string>
#include <codecvt>
#include <format>
#include <print>
#include <algorithm>
#include <unordered_map>

#include "constexpr_intrinsics.h"
#include "constexpr_siphash.h"
#include "Trivial_Span.h"

#include <Windows.h>
#include "MemoryMappedFile.h"
#include <numeric>
#include <algorithm>
#include "MMF_Dense_Sets.h"

#include <zlib/zlib.h>

#include "RenumberedWires.h"

DECLSPEC_NOINLINE std::vector<std::vector<uint32_t>> make_pips(DeviceResources::Device::Reader dev) {

    RenumberedWires wr;

    auto alt_wires{ wr.alt_wires.body };
    auto tiles{ dev.getTileList() };
    auto tileTypes{ dev.getTileTypeList() };

    std::vector<std::vector<uint32_t>> wire_to_pips{ static_cast<size_t>(alt_wires.size()) };
    
    MemoryMappedFile pips_mmf{ L"pips2.bin", 4294967296ui64 };

    auto pips_span{ pips_mmf.get_span<uint64_t>() };

    // std::vector<std::vector<uint64_t>> pips{ 1ui64 };

    puts("start make_pips\n");

    uint32_t pips_size{};
    {
        uint32_t pip_idx{};

        for (auto&& tile : tiles) {
            auto tile_strIdx{ tile.getName() };
            auto tileType{ tileTypes[tile.getType()] };
            auto tileType_wires{ tileType.getWires() };
            for (auto&& pip : tileType.getPips()) {
                if (pip.isPseudoCells()) continue;

                auto wire0_strIdx{ tileType_wires[pip.getWire0()] };
                auto wire1_strIdx{ tileType_wires[pip.getWire1()] };


                // ULARGE_INTEGER key0{ .u{.LowPart{wire0_strIdx}, .HighPart{tile_strIdx}} };
                // ULARGE_INTEGER key1{ .u{.LowPart{wire1_strIdx}, .HighPart{tile_strIdx}} };
                // auto wire0_idx{ tile_strIdx_wire_strIdx_to_wire_idx.at(key0.QuadPart) };
                // auto wire1_idx{ tile_strIdx_wire_strIdx_to_wire_idx.at(key1.QuadPart) };
                auto wire0_idx{ wr.find_wire(tile_strIdx, wire0_strIdx) };
                if (wire0_idx == UINT32_MAX) continue;

                auto wire1_idx{ wr.find_wire(tile_strIdx, wire1_strIdx) };
                if (wire1_idx == UINT32_MAX) continue;

                // ULARGE_INTEGER pipInfo{ .u{.LowPart{wire0_strIdx}, .HighPart{tile_strIdx}} };

                if (pip_idx && !(pip_idx % 1000000)) {
                    std::print("pip_idx: {}M\n", pip_idx / 1000000);
                }

                uint32_t pip_idx_forward{ pip_idx | 0x80000000ui32 };
                uint32_t pip_idx_reverse{ pip_idx };

                uint64_t pip_info{
                    (static_cast<uint64_t>(wire0_idx) & 0xfffffffui64) |
                    ((static_cast<uint64_t>(wire1_idx) & 0xfffffffui64) << 32ui64) |
                    (pip.getDirectional() << 63ui64) //is directional
                };

                pips_span[pip_idx] = pip_info;
                pip_idx++;

                if (pip.getDirectional()) {
                    wire_to_pips[wire0_idx].push_back(pip_idx_forward);
                }
                else {
                    wire_to_pips[wire0_idx].push_back(pip_idx_forward);
                    wire_to_pips[wire1_idx].push_back(pip_idx_reverse);
                }
            }
        }
        pips_size = static_cast<uint64_t>(pip_idx) * sizeof(uint64_t);
    }

    auto pips_mmf_shrunk{ pips_mmf.shrink(pips_size) };

    puts("finish make_pips\n");

    MMF_Dense_Sets_u32::make(L"wire_to_pips2.bin", wire_to_pips);
    // MMF_Dense_Sets_u64::make(L"pips2.bin", pips);
    auto alt_wire_to_pips{ MMF_Dense_Sets_u32{L"wire_to_pips2.bin"} };
    // auto alt_pips{ MMF_Dense_Sets_u64{L"pips2.bin"} };
    alt_wire_to_pips.test(wire_to_pips);
    // alt_pips.test(pips);

    
    MemoryMappedFile wire_to_node_mmf{ L"wire_to_node2.bin", wr.alt_wires.body.size() * sizeof(uint32_t)};
    auto wire_to_node_span{ wire_to_node_mmf.get_span<uint32_t>() };
    for (uint32_t node_idx{}; node_idx < wr.alt_nodes.size(); node_idx++) {
        if (node_idx && !(node_idx % 1000000)) {
            std::print("node_idx: {}M\n", node_idx / 1000000);
        }
        for (auto wire_idx : wr.alt_nodes[node_idx]) {
            wire_to_node_span[wire_idx] = node_idx;
        }
    }

    auto pips_shrunk{ pips_mmf_shrunk.get_span<uint64_t>() };

    OutputDebugStringA("start node_to_pips2\n");
    std::vector<std::vector<uint32_t>> node_to_pips{ wr.alt_nodes.size() };

    for (uint32_t pip_idx{}; pip_idx < pips_shrunk.size(); pip_idx++) {
        if (pip_idx && !(pip_idx % 1000000)) {
            std::print("pip_idx: {}M\n", pip_idx / 1000000);
        }

        auto pip_info{ pips_shrunk[pip_idx] };
        auto wire0{ _bextr_u64(pip_info, 0, 28) };
        auto wire1{ _bextr_u64(pip_info, 32, 28) };
        bool directional{ static_cast<bool>( _bextr_u64(pip_info, 63, 1)) };

        auto node0_idx{ wire_to_node_span[wire0] };
        auto node1_idx{ wire_to_node_span[wire1] };

        node_to_pips[node0_idx].emplace_back(pip_idx);
        if (!directional) {
            node_to_pips[node1_idx].emplace_back(pip_idx);
        }
    }

    OutputDebugStringA("finish node_to_pips2\n");

    MMF_Dense_Sets_u32::make(L"node_to_pips2.bin", node_to_pips);
    auto alt_node_to_pips{ MMF_Dense_Sets_u32{L"node_to_pips2.bin"} };
    alt_node_to_pips.test(node_to_pips);

    return wire_to_pips;
}

DECLSPEC_NOINLINE auto get_tile_strIdx_wire_strIdx_to_wire_idx(DeviceResources::Device::Reader dev) {

    auto wires{ dev.getWires() };
    auto nodes{ dev.getNodes() };

    puts("make tile_strIdx_wire_strIdx_to_wire_idx start\n");

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
    size_t hash_size{ 1ui64 << (64ui64 - _lzcnt_u64(wires.size())) };
    size_t hash_mask{ hash_size - 1ui64 };
    std::print("hash_size: {}, hash_mask: 0x{:x}\n", hash_size, hash_mask);

    std::vector<std::vector<uint64_t>> ret(hash_size);
    // std::vector<std::vector<uint32_t>> wire_renumber(hash_size);
    std::vector<std::vector<uint64_t>> node_renumber(nodes.size());

    for (uint32_t nodeIdx{}; nodeIdx < nodes.size(); nodeIdx++) {
        if (nodeIdx && !(nodeIdx % 1000000)) {
            std::print("nodeIdx: {}M\n", nodeIdx / 1000000);
        }
        std::vector<uint64_t> &node_renumber_n{ node_renumber.at(nodeIdx) };
        auto node_wires{ nodes[nodeIdx].getWires() };
        node_renumber_n.reserve(node_wires.size());
        for (uint32_t node_wireIdx{}; node_wireIdx < node_wires.size(); node_wireIdx++) {
            auto wireIdx{ node_wires[node_wireIdx] };
            auto wire{ wires[wireIdx] };

            ULARGE_INTEGER key{ .u{.LowPart{wire.getWire()}, .HighPart{wire.getTile()}} };
            auto h{ _mm_crc32_u64(0, key.QuadPart) };
            auto ret_index{ h & hash_mask };
            std::vector<uint64_t>& ret_at{ ret.at(ret_index) };
            ULARGE_INTEGER node_key{ .u{.LowPart{static_cast<uint32_t>(ret_index)}, .HighPart{static_cast<uint32_t>(ret_at.size())}}};
            ret_at.emplace_back(key.QuadPart);
            node_renumber_n.emplace_back(node_key.QuadPart);
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
    // MMF_Dense_Sets_u32::make(L"wire_renumber.bin", wire_renumber);
    MMF_Dense_Sets_u64::make(L"wires2.bin", ret);
    auto alt_wires{ MMF_Dense_Sets_u64{L"wires2.bin"} };

    std::vector<std::vector<uint32_t>> node_final(nodes.size());
    for (uint32_t nodeIdx{}; nodeIdx < nodes.size(); nodeIdx++) {
        if (!(nodeIdx % 1000000)) std::print("nodeIdx: {}M\n", nodeIdx / 1000000);
        auto node_wires{ node_renumber.at(nodeIdx) };
        decltype(node_final.at(nodeIdx)) node_final_n{ node_final.at(nodeIdx) };
        node_final_n.resize(node_wires.size());

        for (uint32_t node_wireIdx{}; node_wireIdx < node_wires.size(); node_wireIdx++) {
            auto node_wire_key{ node_wires[node_wireIdx] };
            ULARGE_INTEGER node_key{ .QuadPart{node_wire_key} };
            //.u{ .LowPart{static_cast<uint32_t>(ret_index)}, .HighPart{static_cast<uint32_t>(ret_at.size())} }
            auto ret_index{ node_key.LowPart };
            auto ret_at_size{ node_key.HighPart };
            auto offset{ alt_wires.get_offset(ret_index) + ret_at_size };
            ULARGE_INTEGER wire_found{ .QuadPart{alt_wires.body[offset]}};
            auto wire_found_wire_str{ wire_found.LowPart };
            auto wire_found_tile_str{ wire_found.HighPart };

            auto wire{ wires[nodes[nodeIdx].getWires()[node_wireIdx]] };
            auto wire_str{ wire.getWire() };
            auto wire_tile_str{ wire.getTile() };
            if (wire_found_wire_str != wire_str) {
                std::print("wire_found_wire_str:{}:{} != wire_str:{}:{}\n", wire_found_wire_str, dev.getStrList()[wire_found_wire_str].cStr(), wire_str, dev.getStrList()[wire_str].cStr());
                DebugBreak();
            }
            if (wire_found_tile_str != wire_tile_str) {
                std::print("wire_found_tile_str:{}:{} != wire_tile_str:{}:{}\n", wire_found_tile_str, dev.getStrList()[wire_found_tile_str].cStr(), wire_tile_str, dev.getStrList()[wire_tile_str].cStr());
                DebugBreak();
            }
            node_final_n[node_wireIdx]=offset;
        }
    }

    MMF_Dense_Sets_u32::make(L"nodes2.bin", node_final);
    auto alt_nodes{ MMF_Dense_Sets_u32{L"nodes2.bin"} };


    puts("make tile_strIdx_wire_strIdx_to_wire_idx finish\n");
    return ret;
#endif
}

DECLSPEC_NOINLINE void make_pips(std::wstring gzPath) {
	MemoryMappedFile mmf_phys_gz{ gzPath };
	MemoryMappedFile mmf_phys{ L"temp_unzipped.bin", 4294967296ui64 };

	auto read_span{ mmf_phys_gz.get_span<Bytef>() };
	auto write_span{ mmf_phys.get_span<Bytef>() };
	z_stream strm{
		.next_in{read_span.data()},
		.avail_in{static_cast<uint32_t>(read_span.size())},
		.next_out{write_span.data()},
		.avail_out{UINT32_MAX},
	};

	auto init_result{ inflateInit2(&strm, 15 + 16) };
	auto inflate_result{ inflate(&strm, Z_FINISH) };
	auto end_result{ inflateEnd(&strm) };
	std::print("init_result: {}\ninflate_result: {}\nend_result: {}\n", init_result, inflate_result, end_result);
	auto mmf_unzipped{ mmf_phys.shrink(strm.total_out) };


	auto span_words{ mmf_unzipped.get_span<capnp::word>() };
	kj::ArrayPtr<capnp::word> words{ span_words.data(), span_words.size() };
	capnp::FlatArrayMessageReader famr{ words, {.traversalLimitInWords = UINT64_MAX, .nestingLimit = INT32_MAX} };
	auto dev{ famr.getRoot<DeviceResources::Device>() };
	mmf_unzipped.reopen_delete();

    std::print("tile_count: {}, bit_count: {}\n", dev.getTileList().size(), ceil(log2(dev.getTileList().size())));
    std::print("wire_count: {}, bit_count: {}\n", dev.getWires().size(), ceil(log2(dev.getWires().size())));
    std::print("node_count: {}, bit_count: {}\n", dev.getNodes().size(), ceil(log2(dev.getNodes().size())));

    // auto tile_strIdx_wire_strIdx_to_wire_idx{ get_tile_strIdx_wire_strIdx_to_wire_idx(dev) };

    // make_pips(dev);

    RenumberedWires wr;
    wr.test_nodes(dev);
    wr.test_wires(dev);
}

int main(int argc, char* argv[]) {
	make_pips(L"_deps/device-file-src/xcvu3p.device");
	return 0;
}