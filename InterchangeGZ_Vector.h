#include "DeviceResources.capnp.h"
#include "LogicalNetlist.capnp.h"
#include "PhysicalNetlist.capnp.h"
#include <capnp/serialize.h>
#include <zlib.h>
#include "MemoryMappedFile.h"
#include <vector>

template<typename T>
class InterchangeGZ_Vector {
public:
	std::vector<capnp::word> v_unzipped;
	kj::ArrayPtr<capnp::word> words;
	capnp::FlatArrayMessageReader famr;
	T::Reader root;

	static uint32_t get_gz_isize(MemoryMappedFile& mmf_gz) {
		auto bytes{ mmf_gz.get_span<uint8_t>(mmf_gz.fsize - 4ull, 4) };
		std::array<uint8_t, 4> last_4_bytes{};
		std::ranges::copy(bytes, last_4_bytes.begin());
		auto ret{ std::bit_cast<uint32_t>(last_4_bytes) };
		return ret;
	}

	static std::vector<capnp::word> inflate_v(std::string fn) {
		MemoryMappedFile mmf_gz{ fn };
		auto v_unzipped{ std::vector<capnp::word>(static_cast<size_t>(get_gz_isize(mmf_gz)) / sizeof(capnp::word)) };
		std::span<capnp::word> s_unzipped(v_unzipped);

		auto read_span{ mmf_gz.get_span<Bytef>() };
		auto write_span{ std::span<Bytef>(reinterpret_cast<Bytef*>(s_unzipped.data()), s_unzipped.size_bytes()) };
		z_stream strm{
			.next_in{read_span.data()},
			.avail_in{static_cast<uint32_t>(read_span.size())},
			.next_out{write_span.data()},
			.avail_out{static_cast<uint32_t>(write_span.size())},
		};

		auto init_result{ inflateInit2(&strm, 15 + 16) };
		if (init_result != Z_OK) abort();

		auto inflate_result{ inflate(&strm, Z_FINISH) };
		if (inflate_result != Z_STREAM_END) abort();

		auto end_result{ inflateEnd(&strm) };
		if (end_result != Z_OK) abort();

#ifdef _DEBUG
		puts(std::format("init_result: {}\ninflate_result: {}\nend_result: {}\n", init_result, inflate_result, end_result).c_str());
#endif
		//auto mmf_unzipped{ mmf_temp.shrink(strm.total_out) };

		//return mmf_unzipped;
		return v_unzipped;
	}
	InterchangeGZ_Vector() = default;

	InterchangeGZ_Vector(std::string fn) noexcept :
		v_unzipped{ inflate_v(fn) },
		words{ v_unzipped.data(), v_unzipped.size() },
		famr{ words, {.traversalLimitInWords = UINT64_MAX, .nestingLimit = INT32_MAX} },
		root{ famr.getRoot<T>() }
	{ }

	static size_t write(std::string gz_name, ::capnp::MallocMessageBuilder& message, int level = Z_NO_COMPRESSION) noexcept {
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

using DevGZV = InterchangeGZ_Vector<DeviceResources::Device>;
using PhysGZV = InterchangeGZ_Vector<PhysicalNetlist::PhysNetlist>;

