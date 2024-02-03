#include "MemoryMappedFile.h"
#include <zlib.h>

class InflateGZ {
public:
	MemoryMappedFile mmf_unzipped;

	static uint32_t get_gz_isize(MemoryMappedFile& mmf) {
		auto bytes{ mmf.get_span<uint8_t>(mmf.fsize - 4ull, 4) };
		std::array<uint8_t, 4> last_4_bytes{};
		std::ranges::copy(bytes, last_4_bytes.begin());
		auto ret{ std::bit_cast<uint32_t>(last_4_bytes) };
		return ret;
	}

	static MemoryMappedFile inflate_mmf(std::string fn_src, std::string fn_dst) {
		MemoryMappedFile mmf_gz{ fn_src };
		MemoryMappedFile mmf_dst{ fn_dst, static_cast<uint64_t>(get_gz_isize(mmf_gz))};

		auto read_span{ mmf_gz.get_span<uint8_t>() };
		auto write_span{ mmf_dst.get_span<uint8_t>() };

		z_stream strm{
			.next_in{read_span.data()},
			.avail_in{static_cast<uint32_t>(read_span.size())},
			.next_out{write_span.data()},
			.avail_out{UINT32_MAX},
		};

		auto init_result{ inflateInit2(&strm, 15 + 16) };
		if (init_result != Z_OK) abort();

		auto inflate_result{ inflate(&strm, Z_FINISH) };
		if (inflate_result != Z_STREAM_END) abort();

		auto end_result{ inflateEnd(&strm) };
		if (end_result != Z_OK) abort();

		return mmf_dst;
	}
};

