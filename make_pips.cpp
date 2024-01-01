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
#include <queue>

#include "RenumberedWires.h"

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

	RenumberedWires::make_pips(dev);
}

int main(int argc, char* argv[]) {
	make_pips(L"_deps/device-file-src/xcvu3p.device");
	return 0;
}