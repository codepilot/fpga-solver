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

DECLSPEC_NOINLINE std::vector<std::vector<uint32_t>> make_vv_wire_idx_pip_wire_idx(DeviceResources::Device::Reader dev, std::unordered_map<uint64_t, uint32_t> tile_strIdx_wire_strIdx_to_wire_idx) {

    auto wires{ dev.getWires() };
    auto tiles{ dev.getTileList() };
    auto tileTypes{ dev.getTileTypeList() };

    std::vector<std::vector<uint32_t>> ret{ static_cast<size_t>(wires.size()) };

    puts("start make_vv_wire_idx_pip_wire_idx\n");

    for (auto&& tile : tiles) {
        auto tile_strIdx{ tile.getName() };
        auto tileType{ tileTypes[tile.getType()] };
        auto tileType_wires{ tileType.getWires() };
        for (auto&& pip : tileType.getPips()) {
            if (pip.isPseudoCells()) continue;

            auto wire0_strIdx{ tileType_wires[pip.getWire0()] };
            auto wire1_strIdx{ tileType_wires[pip.getWire1()] };
            ULARGE_INTEGER key0{ .u{.LowPart{wire0_strIdx}, .HighPart{tile_strIdx}} };
            ULARGE_INTEGER key1{ .u{.LowPart{wire1_strIdx}, .HighPart{tile_strIdx}} };
            auto wire0_idx{ tile_strIdx_wire_strIdx_to_wire_idx.at(key0.QuadPart) };
            auto wire1_idx{ tile_strIdx_wire_strIdx_to_wire_idx.at(key1.QuadPart) };
            if (pip.getDirectional()) {
                //ret.insert({ wire0_idx, wire1_idx });
                ret[wire0_idx].push_back(wire1_idx);
            }
            else {
                //                    ret.insert({ wire0_idx, wire1_idx });
                //                    ret.insert({ wire1_idx, wire0_idx });
                ret[wire0_idx].push_back(wire1_idx);
                // ret[wire1_idx].push_back(wire0_idx);
            }
        }
    }

    puts("finish make_vv_wire_idx_pip_wire_idx\n");
    MMF_Dense_Sets_u32::make(L"vv_wire_idx_pip_wire_idx.bin", ret);
    auto alt{ MMF_Dense_Sets_u32{L"vv_wire_idx_pip_wire_idx.bin"} };
    alt.test(ret);

    return ret;
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

    RenumberedWires wr;
    wr.test_nodes(dev);
    wr.test_wires(dev);
    // make_vv_wire_idx_pip_wire_idx(dev, );
}

int main(int argc, char* argv[]) {
	make_pips(L"_deps/device-file-src/xcvu3p.device");
	return 0;
}