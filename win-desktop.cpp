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
	thread_local GL46* gl{};


#include "constexpr_intrinsics.h"
#include "Trivial_Span.h"

#include "constexpr_siphash.h"
#include "mmf.h"
#include <winioctl.h>
#include "MemoryMappedFile.h"
#include "cached_node_lookup.h"

#include "Route_Phys.h"


#include "MMF_Dense_Sets.h"
// #include "dev.h"
// #include "route_options.h"
// #include "Physnet.h"

#include "png.h"

#include "GL46.h"

constexpr int numElements = 32;

int test_opencl() {
    // Filter for a 2.0 or newer platform and set it as the default
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    cl::Platform plat;
    for (auto& p : platforms) {
        std::string platver = p.getInfo<CL_PLATFORM_VERSION>();
        if (platver.find("OpenCL 2.") != std::string::npos ||
            platver.find("OpenCL 3.") != std::string::npos) {
            // Note: an OpenCL 3.x platform may not support all required features!
            plat = p;
        }
    }
    if (plat() == 0) {
        OutputDebugStringA(std::format("No OpenCL 2.0 or newer platform found.\n").c_str());
        return -1;
    }

    cl::Platform newP = cl::Platform::setDefault(plat);
    if (newP != plat) {
        OutputDebugStringA(std::format("Error setting default platform.\n").c_str());
        return -1;
    }

    // C++11 raw string literal for the first kernel
    std::string kernel1{ R"CLC(
        global int globalA;
        kernel void updateGlobal()
        {
          globalA = 75;
        }
    )CLC" };

    // Raw string literal for the second kernel
    std::string kernel2{ R"CLC(
        __kernel void square(__global int* input, __global int* output, const unsigned int count) {
            int i = get_global_id(0);
            if(i < count) output[i] = input[i] * input[i];
        }
    )CLC" };

    std::vector<std::string> programStrings;
    programStrings.push_back(kernel1);
    programStrings.push_back(kernel2);

    cl::Program square_Program(programStrings);
    try {
        square_Program.build("-cl-std=CL3.0");
    }
    catch (...) {
        // Print build info for all devices
        cl_int buildErr = CL_SUCCESS;
        auto buildInfo = square_Program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(&buildErr);
        for (auto& pair : buildInfo) {
            OutputDebugStringA(std::format("{}\n\n", pair.second).c_str());
        }

        return 1;
    }

    typedef struct { int* bar; } Foo;

    // Get and run kernel that initializes the program-scope global
    // A test for kernels that take no arguments
    auto program2Kernel =
        cl::KernelFunctor<>(square_Program, "square");
    program2Kernel(
        cl::EnqueueArgs(
            cl::NDRange(1)));

    //////////////////
    // SVM allocations

    auto anSVMInt = cl::allocate_svm<int, cl::SVMTraitCoarse<>>();
    *anSVMInt = 5;
    cl::SVMAllocator<Foo, cl::SVMTraitCoarse<cl::SVMTraitReadOnly<>>> svmAllocReadOnly;
    auto fooPointer = cl::allocate_pointer<Foo>(svmAllocReadOnly);
    fooPointer->bar = anSVMInt.get();
    cl::SVMAllocator<int, cl::SVMTraitCoarse<>> svmAlloc;
    std::vector<int, cl::SVMAllocator<int, cl::SVMTraitCoarse<>>> inputA(numElements, 1, svmAlloc);
    cl::coarse_svm_vector<int> inputB(numElements, 2, svmAlloc);

    //////////////
    // Traditional cl_mem allocations

    std::vector<int> output(numElements, 0xdeadbeef);
    cl::Buffer outputBuffer(output.begin(), output.end(), false);

    // Default command queue, also passed in as a parameter
    cl::DeviceCommandQueue defaultDeviceQueue = cl::DeviceCommandQueue::makeDefault(
        cl::Context::getDefault(), cl::Device::getDefault());

    auto square_Kernel =
        cl::KernelFunctor<
        int*,
        cl::Buffer,
        int,
        cl::DeviceCommandQueue
        >(square_Program, "square");

    // Ensure that the additional SVM pointer is available to the kernel
    // This one was not passed as a parameter
    square_Kernel.setSVMPointers(anSVMInt);

    cl_int error;
    square_Kernel(
        cl::EnqueueArgs(
            cl::NDRange(numElements / 2),
            cl::NDRange(numElements / 2)),
        inputB.data(),
        outputBuffer,
        3,
        defaultDeviceQueue,
        error
    );

    cl::copy(outputBuffer, output.begin(), output.end());

    cl::Device d = cl::Device::getDefault();

    OutputDebugStringA(std::format("Output:\n").c_str());
    for (int i = 1; i < numElements; ++i) {
        OutputDebugStringA(std::format("\t{}\n", output[i]).c_str());
    }
    OutputDebugStringA(std::format("\n\n").c_str());
    return 0;
}

int32_t WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int32_t nShowCmd) {
    // test_opencl()
	GL46 gl46;
	gl46.loop();

	return 0;
}
