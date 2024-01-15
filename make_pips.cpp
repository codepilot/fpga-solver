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
#include <vector>
#include "MMF_Dense_Sets.h"

#include <zlib/zlib.h>
#include <queue>

#include "RenumberedWires.h"
#include <cmath>
#include "InterchangeGZ.h"
#include <thread>
#include <barrier>

void make_pips(std::string gzPath) {
	DevGZ devGZ{ gzPath, false };
	auto dev{ devGZ.root };

    puts(std::format("tile_count: {}, bit_count: {}", dev.getTileList().size(), ceil(log2(dev.getTileList().size()))).c_str());
    puts(std::format("wire_count: {}, bit_count: {}", dev.getWires().size(), ceil(log2(dev.getWires().size()))).c_str());
    puts(std::format("node_count: {}, bit_count: {}", dev.getNodes().size(), ceil(log2(dev.getNodes().size()))).c_str());

	RenumberedWires::make_pips(dev);
	RenumberedWires::test(dev);
}

int main(int argc, char* argv[]) {
	make_pips("_deps/device-file-src/xcvu3p.device");
	return 0;
}