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

#include <safeint.h>
#include "InterchangeGZ.h"

#include "Route_Phys.h"

int main(int argc, char* argv[]) {
	Route_Phys rp;
	rp.route();

	return 0;
}