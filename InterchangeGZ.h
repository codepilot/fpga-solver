#pragma once

template<typename T>
class InterchangeGZ {
public:
	MemoryMappedFile mmf_unzipped;
	std::span<capnp::word> span_words;
	kj::ArrayPtr<capnp::word> words;
	capnp::FlatArrayMessageReader famr;
	T::Reader root;

	static MemoryMappedFile inflate_mmf(std::wstring fn) {
		MemoryMappedFile mmf_gz{ fn };
		MemoryMappedFile mmf_temp{ fn + L".temp", 4294967296ui64};

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
		std::print("init_result: {}\ninflate_result: {}\nend_result: {}\n", init_result, inflate_result, end_result);
		auto mmf_unzipped{ mmf_temp.shrink(strm.total_out) };

		mmf_unzipped.reopen_delete();
		return mmf_unzipped;
	}

	InterchangeGZ(std::wstring fn) :
		mmf_unzipped{ inflate_mmf(fn) },
		span_words{ mmf_unzipped.get_span<capnp::word>() },
		words{ span_words.data(), span_words.size() },
		famr{ words, {.traversalLimitInWords = UINT64_MAX, .nestingLimit = INT32_MAX} },
		root{ famr.getRoot<T>() }
	{ }

	DECLSPEC_NOINLINE static void write(::capnp::MallocMessageBuilder &message) {
		size_t gzSize{};
		size_t msgSize{};
		auto fa{ messageToFlatArray(message) };
		auto fa_bytes{ fa.asBytes() };
		msgSize = fa_bytes.size();

		z_stream strm{
			.next_in{reinterpret_cast<Bytef*>(fa.begin())},
			.avail_in{msl::utilities::SafeInt<uint32_t>(msgSize)},
		};

		deflateInit2(&strm, Z_NO_COMPRESSION, Z_DEFLATED, 16 | 15, 9, Z_DEFAULT_STRATEGY);

		MemoryMappedFile dst_written{ L"dst_written.phy.gz", deflateBound(&strm, msgSize) };
		strm.next_out = reinterpret_cast<Bytef*>(dst_written.fp);
		strm.avail_out = dst_written.fsize;
		deflate(&strm, Z_FINISH);
		deflateEnd(&strm);

		gzSize = strm.total_out;
		auto shrunk_written{ dst_written.shrink(gzSize) };

	}
};

using DevGZ = InterchangeGZ<DeviceResources::Device>;
using PhysGZ = InterchangeGZ<PhysicalNetlist::PhysNetlist>;
