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

#include <zlib/zlib.h>
#include <queue>

#include "RenumberedWires.h"
#include "RouteStorage.h"

#ifdef _WIN32
#include <safeint.h>
#endif
#include "InterchangeGZ.h"
#include <unordered_set>

#include "Route_Phys.h"

int main(int argc, char* argv[]) {
	Route_Phys rp;
	// rp.start_routing({}, {}, {});
	rp.make_search_files();
	rp.tile_based_routing();
	// rp.jt.join();
	return 0;
}