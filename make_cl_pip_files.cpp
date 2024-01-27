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
#include <vector>
#ifdef _WIN32
#include <print>
#endif
#include <algorithm>
#include <unordered_map>

#ifdef _WIN32
#include "constexpr_intrinsics.h"
#include "constexpr_siphash.h"
#endif
#include "Trivial_Span.h"

#ifdef _WIN32
#include <Windows.h>
#endif
#include "MemoryMappedFile.h"
#include <numeric>
#include <algorithm>
#include "MMF_Dense_Sets.h"

#include <zlib.h>
#include <queue>

#include "RenumberedWires.h"
#include "RouteStorage.h"

#ifdef _WIN32
#include <safeint.h>
#endif
#include "InterchangeGZ.h"
#include <unordered_set>

#include "Route_Phys.h"

class inspect_pip_files {
public:
	static inline const MemoryMappedFile mmf_v_pip_count_offset{ "pip_count_offset.bin" };
	static inline const MemoryMappedFile mmf_v_pip_tile_body{ "pip_tile_body.bin" };
	static inline const MemoryMappedFile mmf_v_pip_body{ "pip_body.bin" };
	static inline const std::span<std::array<uint32_t, 2>> s_pip_count_offset{ mmf_v_pip_count_offset.get_span<std::array<uint32_t, 2>>() };
	static inline const std::span<std::array<uint32_t, 4>> s_pip_tile_body{ mmf_v_pip_tile_body.get_span<std::array<uint32_t, 4>>() };
	static inline const std::span<std::array<uint32_t, 4>> s_pip_body{ mmf_v_pip_body.get_span<std::array<uint32_t, 4>>() };
	static inline void inspect() {
		double average_count{ static_cast<double>(s_pip_tile_body.size()) / static_cast<double>(s_pip_count_offset.size()) };
		std::cout << std::format("average_count: {}\n", average_count);

		uint32_t max_count{};
		uint32_t min_count{ UINT32_MAX };
		for (auto&& count_offset : s_pip_count_offset) {
			max_count = std::max(max_count, count_offset[0]);
			min_count = std::min(min_count, count_offset[0]);
		}
		std::cout << std::format("s_pip_tile_body.size(): {}\n", s_pip_tile_body.size());
		std::cout << std::format("min_count: {}, max_count: {}\n", min_count, max_count);
		return;
	}
};

int main(int argc, char* argv[]) {
	Route_Phys::make_cl_pip_files();
	inspect_pip_files::inspect();
	return 0;
}