#define NOMINMAX

#include <cstdint>
#include <vector>
#include <span>
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

#include <CL/opencl.hpp>

#pragma comment( lib, "Opengl32" )
#pragma comment( lib, "Kernel32" )
#pragma comment( lib, "WindowsApp" )
#pragma comment( lib, "Avrt" )
#pragma comment( lib, "OpenCL" )

namespace {
	using msl::utilities::SafeInt;
	class GL46;
	thread_local GL46* gl{};

#include "GL46.h"
};

int32_t WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int32_t nShowCmd) {
	GL46 gl46;
	gl46.loop();

	return 0;
}
