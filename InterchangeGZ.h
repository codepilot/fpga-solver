#pragma once

#include "DeviceResources.capnp.h"
#include "LogicalNetlist.capnp.h"
#include "PhysicalNetlist.capnp.h"
#include <capnp/serialize.h>
#include <zlib.h>

template<typename T>
class InterchangeGZ {
public:
	MemoryMappedFile mmf_unzipped;
	std::span<capnp::word> span_words;
	kj::ArrayPtr<capnp::word> words;
	capnp::FlatArrayMessageReader famr{ kj::ArrayPtr<const capnp::word>{} };
	T::Reader root;

	static MemoryMappedFile inflate_mmf(std::string fn, bool delete_temp = true) {
		MemoryMappedFile mmf_gz{ fn };
		MemoryMappedFile mmf_temp{ fn + ".temp", 4294967296ull};

		auto read_span{ mmf_gz.get_span<Bytef>() };
		auto write_span{ mmf_temp.get_span<Bytef>() };
		z_stream strm{
			.next_in{read_span.data()},
			.avail_in{static_cast<uint32_t>(read_span.size())},
			.next_out{write_span.data()},
			.avail_out{UINT32_MAX},
		};

		auto init_result{ inflateInit2(&strm, 15 + 16) };
		auto inflate_result{ inflate(&strm, Z_FINISH) };
		auto end_result{ inflateEnd(&strm) };
#ifdef _DEBUG
		puts(std::format("init_result: {}\ninflate_result: {}\nend_result: {}\n", init_result, inflate_result, end_result).c_str());
#endif
		auto mmf_unzipped{ mmf_temp.shrink(strm.total_out) };

		if(delete_temp) mmf_unzipped.reopen_delete();
		return mmf_unzipped;
	}
	InterchangeGZ() {

	}

	InterchangeGZ(InterchangeGZ&& other):
		mmf_unzipped{ std::move(other.mmf_unzipped) },
		span_words{ mmf_unzipped.get_span<capnp::word>() },
		words{ span_words.data(), span_words.size() },
		famr{ words, {.traversalLimitInWords = UINT64_MAX, .nestingLimit = INT32_MAX} },
		root{ famr.getRoot<T>() }
	{

	}

	InterchangeGZ(std::string fn, bool delete_temp = true) :
		mmf_unzipped{ inflate_mmf(fn, delete_temp) },
		span_words{ mmf_unzipped.get_span<capnp::word>() },
		words{ span_words.data(), span_words.size() },
		famr{ words, {.traversalLimitInWords = UINT64_MAX, .nestingLimit = INT32_MAX} },
		root{ famr.getRoot<T>() }
	{ }

	static size_t write(std::string gz_name, ::capnp::MallocMessageBuilder &message, int level = Z_NO_COMPRESSION) {
		size_t gzSize{};
		size_t msgSize{};
		auto fa{ messageToFlatArray(message) };
		auto fa_bytes{ fa.asBytes() };
		msgSize = fa_bytes.size();

		z_stream strm{
			.next_in{reinterpret_cast<Bytef*>(fa.begin())},
			.avail_in{static_cast<uint32_t>(msgSize)},
		};

		deflateInit2(&strm, level, Z_DEFLATED, 16 | 15, 9, Z_DEFAULT_STRATEGY);

		MemoryMappedFile dst_written{ gz_name, deflateBound(&strm, static_cast<uLong>(msgSize)) };
		strm.next_out = reinterpret_cast<Bytef*>(dst_written.fp);
		strm.avail_out = static_cast<uInt>(dst_written.fsize);
		deflate(&strm, Z_FINISH);
		deflateEnd(&strm);

		gzSize = strm.total_out;
		auto shrunk_written{ dst_written.shrink(gzSize) };
		return gzSize;
	}
};

template<typename T>
class InterchangeFlat {
public:
	MemoryMappedFile mmf_unzipped;
	std::span<capnp::word> span_words;
	kj::ArrayPtr<capnp::word> words;
	capnp::FlatArrayMessageReader famr;
	T::Reader root;

	InterchangeFlat(std::string fn, bool delete_temp = true) :
		mmf_unzipped{ fn + ".temp" },
		span_words{ mmf_unzipped.get_span<capnp::word>() },
		words{ span_words.data(), span_words.size() },
		famr{ words, {.traversalLimitInWords = UINT64_MAX, .nestingLimit = INT32_MAX} },
		root{ famr.getRoot<T>() }
	{ }

};

using DevGZ = InterchangeGZ<DeviceResources::Device>;
using PhysGZ = InterchangeGZ<PhysicalNetlist::PhysNetlist>;

using DevFlat = InterchangeFlat<DeviceResources::Device>;
