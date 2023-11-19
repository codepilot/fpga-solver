#include <capnp/serialize.h>

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

#include <zlib/zlib.h>

int main(int argc, char* argv[]) {
	std::vector<std::wstring> wargs;
	std::vector<std::string_view> args;
	args.reserve(argc);
	for (int i{}; i != argc; i++) {
		args.emplace_back(argv[i]);
	}
	if(args.size() != 4) return 1;
	for(auto &&arg: args) {
		wargs.emplace_back(std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(std::string{ arg }));
	}

	{
		MemoryMappedFile mmf_args{ wargs.at(1) };
		MemoryMappedFile mmf_dst{ wargs.at(2), 4294967296ui64 };
		auto read_span{ mmf_args.get_span<Bytef>() };
		auto write_span{ mmf_dst.get_span<Bytef>()};
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
		auto mmf_unzipped{ mmf_dst.shrink(strm.total_out) };
		auto span_words{ mmf_unzipped.get_span<capnp::word>() };
		kj::ArrayPtr<capnp::word> words{ span_words.data(), span_words.size()};
		capnp::FlatArrayMessageReader famr{ words, {.traversalLimitInWords = UINT64_MAX, .nestingLimit = INT32_MAX} };
		auto anyReader{ famr.getRoot<capnp::AnyStruct>() };

		MemoryMappedFile mmf_canon_dst{ wargs.at(3), mmf_unzipped.fsize };
		auto mmf_canon_dst_span{ mmf_canon_dst.get_span<capnp::word>() };
		kj::ArrayPtr<capnp::word> backing{ mmf_canon_dst_span.data(), mmf_canon_dst_span.size() };
		auto canonical_size = anyReader.canonicalize(backing);
		auto mmf_canon_dst_shrunk{ mmf_canon_dst.shrink(canonical_size * sizeof(capnp::word)) };
		std::print("canon_size:   {}\n", mmf_canon_dst_shrunk.fsize);
		mmf_unzipped.will_delete = true;
		std::print("mmf_unzipped: {}\n", mmf_unzipped.fsize);
	}
	{
		MemoryMappedFile mmf_canon_dst{ wargs.at(3) };

		auto span_words{ mmf_canon_dst.get_span<capnp::word>() };
		kj::ArrayPtr<capnp::word> words{ span_words.data(), span_words.size() };

		kj::ArrayPtr<const capnp::word> segments[1] = { words };
		capnp::SegmentArrayMessageReader message(segments, { .traversalLimitInWords = UINT64_MAX, .nestingLimit = INT32_MAX });
		auto isCanon{ message.isCanonical() };
		std::print("isCanon: {}\n", isCanon);

	}

	return 0;
}