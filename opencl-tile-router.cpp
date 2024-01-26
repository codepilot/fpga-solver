// #define USE_CPP_INSTEAD_OF_OPENCL

#include "ocl_tile_router.h"
#include <chrono>

#include "constexpr_string.h"
#include "Timer.h"

int main(int argc, char* argv[]) {
	std::vector<std::string> args;
	for (auto &&arg: std::span<char*>(argv, static_cast<size_t>(argc))) args.emplace_back(arg);

	auto phys_file{ (args.size() >= 2) ? args.at(1) : "_deps/benchmark-files-src/mlcad_d181_lefttwo3rds_unrouted.phys" };

	std::cout << std::format("Routing {}\n", phys_file);

	auto dev{ TimerVal(DevFlat("_deps/device-file-src/xcvu3p.device" )) };
	auto phys{ TimerVal(PhysGZ(phys_file)) };

	std::vector<cl_context_properties> context_properties;

	ocl::platform::each([&](uint64_t platform_idx, ocl::platform &platform) {
		platform.each_device<CL_DEVICE_TYPE_GPU>([&](uint64_t device_idx, ocl::device device) {
			if (context_properties.empty()) {
				context_properties.emplace_back(CL_CONTEXT_PLATFORM);
				context_properties.emplace_back(std::bit_cast<cl_context_properties>(platform.platform));
			}
		});
	});
	if (!context_properties.empty()) {
		context_properties.emplace_back(0);
	}

	auto ocltr{ TimerVal(OCL_Tile_Router::make(dev.root, phys.root, context_properties)) };
	TimerVal(ocltr.do_all()).value();
	puts("complete\n");
}