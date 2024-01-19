#include <capnp/serialize.h>
#include "DeviceResources.capnp.h"
#include "LogicalNetlist.capnp.h"
#include "PhysicalNetlist.capnp.h"


#include <cstdint>
#include <vector>
#include <numeric>
#include <span>
#include <queue>
#include <optional>
#include <array>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <GL/glcorearb.h>
#include <GL/glext.h>
#include <GL/wglext.h>
#include <safeint.h>
#include <algorithm>    // std::shuffle
#include <random>       // std::default_random_engine
#include <chrono>       // std::chrono::system_clock
#include <strsafe.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <avrt.h>
#include <Mmreg.h>
#include <immintrin.h>
#include <numbers>
#include <format>
#include <fstream>

#include <CL/opencl.hpp>

#include <zlib/zlib.h>

using msl::utilities::SafeInt;
class GL46;

#include "constexpr_intrinsics.h"
#include "Trivial_Span.h"

#include "constexpr_siphash.h"
#include "mmf.h"
#include <winioctl.h>
#include "MemoryMappedFile.h"
#include "cached_node_lookup.h"

// #include "Route_Phys.h"


#include "MMF_Dense_Sets.h"

#include "png.h"

#include "GL46.h"

int32_t WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int32_t nShowCmd) {
    // test_opencl()
	GL46 gl46;
	gl46.loop();

	return 0;
}
