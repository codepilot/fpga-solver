#include <capnp/serialize.h>
#include "generated\DeviceResources.capnp.h"
#include "generated\LogicalNetlist.capnp.h"
#include "generated\PhysicalNetlist.capnp.h"


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

#include "constexpr_intrinsics.h"
#include "constexpr_siphash.h"
#include "Trivial_Span.h"

#include <Windows.h>
#include "MemoryMappedFile.h"
#include "cached_node_lookup.h"

#include <zlib/zlib.h>

int main(int argc, char* argv[]) {
	std::vector<std::string_view> args;
	std::vector<std::wstring> wargs;
	for (int i{}; i != argc; i++) {
		args.emplace_back(argv[i]);
	}
	if (args.size() != 5) return 1;
	for (auto&& arg : args) {
		wargs.emplace_back(std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(std::string{ arg }));
	}
	{
		int32_t arg_index{};
		for (auto&& arg : args) {
			std::print("[{}]{} ", arg_index, arg);
			arg_index++;
		}
		std::print("\n");
	}

	MemoryMappedFile mmf2{ wargs.at(2), 65536 };
	MemoryMappedFile mmf3{ wargs.at(3), 65536 };
	MemoryMappedFile mmf4{ wargs.at(4), 65536 };

#if 0
#endif
#if 0
	{
		std::string file_name{ args.size() > 1 ? args.at(1) : "C:/Users/root/Desktop/fpga-solver/build/_deps/device-file-src/xcvu3p.device" };
		std::wstring wfile_name{ std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(file_name)};
		MemoryMappedFile mmf_args{ wfile_name };
		MemoryMappedFile mmf_dst{ L"cache/unzipped.device", 3221225472ui64 };
		auto read_span{ mmf_args.get_span<Bytef>() };
		auto write_span{ mmf_dst.get_span<Bytef>()};
		z_stream strm{
			 .next_in{read_span.data()},
			.avail_in{static_cast<uint32_t>(read_span.size())},
			.next_out{write_span.data()},
			.avail_out{static_cast<uint32_t>(write_span.size())},
		};
		auto init_result{ inflateInit2(&strm, 15 + 16) };
		auto inflate_result{ inflate(&strm, Z_FINISH) };
		auto end_result{ inflateEnd(&strm) };
		std::print("init_result: {}\ninflate_result: {}\nend_result: {}\n", init_result, inflate_result, end_result);
		auto mmf_unzipped{ mmf_dst.shrink(strm.total_out) };
		std::print("mmf_unzipped: {}\n", mmf_unzipped.fsize);
		auto span_words{ mmf_unzipped.get_span<capnp::word>() };
		kj::ArrayPtr<capnp::word> words{ span_words.data(), span_words.size()};
		capnp::FlatArrayMessageReader famr{ words, {.traversalLimitInWords = UINT64_MAX, .nestingLimit = INT32_MAX} };
		capnp::AnyStruct::Reader anyReader{ famr.getRoot<capnp::AnyStruct>()};

		auto canonical2 = anyReader.canonicalize();
		const auto bytes{ canonical2.asBytes() };

		MemoryMappedFile mmf_canon_dst{ L"cache/unzipped.canon.device", bytes.size() };
		memcpy(mmf_canon_dst.fp, bytes.begin(), bytes.size());
		std::print("unzipped.canon.device: {}\n", bytes.size());
	}
	{
		MemoryMappedFile mmf_canon_dst{ L"cache/unzipped.canon.device" };

		// Technically we don't know if the bytes are aligned so we'd better copy them to a new
		// array. Note that if we have a non-whole number of words we chop off the straggler
		// bytes. This is fine because if those bytes are actually part of the message we will
		// hit an error later and if they are not then who cares?

		auto span_words{ mmf_canon_dst.get_span<capnp::word>() };
		kj::ArrayPtr<capnp::word> words{ span_words.data(), span_words.size() };

		kj::ArrayPtr<const capnp::word> segments[1] = { words };
		capnp::SegmentArrayMessageReader message(segments, { .traversalLimitInWords = UINT64_MAX, .nestingLimit = INT32_MAX });
		auto isCanon{ message.isCanonical() };
		std::print("isCanon: {}\n", isCanon);

	}
#endif
	return 0;
}