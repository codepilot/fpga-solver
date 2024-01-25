// #define USE_CPP_INSTEAD_OF_OPENCL

#include "ocl_tile_router.h"
#include <chrono>

#include "constexpr_string.h"

template<constexpr_string str>
class Timer {
public:
	const decltype(std::chrono::steady_clock::now()) start;
	Timer() : start{ std::chrono::steady_clock::now() } { }
	std::chrono::duration<double> finish() {
		const auto end = std::chrono::steady_clock::now();
		const auto diff = end - start;
		return diff;
	}
	~Timer() {
		std::cout << std::format(str.chars, finish());
	}
	static decltype(auto) log(auto lamda) {
		auto execLambda{ Timer() };
		decltype(auto) ret = lamda();
		return ret;
	}
};

#define StringOf2(arg) #arg
#define StringOf(arg) StringOf2(arg)
#define TimerVal(arg) Timer<("{:10} @ " __FILE__ ":" StringOf(__LINE__) " " #arg "\n")>::log([&]() { return arg; })

int main(int argc, char* argv[]) {
	std::vector<std::string> args;
	for (auto &&arg: std::span<char*>(argv, static_cast<size_t>(argc))) args.emplace_back(arg);

	auto phys_file{ (args.size() >= 2) ? args.at(1) : "_deps/benchmark-files-src/corescore_500_pb_unrouted.phys" };

	std::cout << std::format("Routing {}\n", phys_file);

	auto dev{ TimerVal(DevFlat("_deps/device-file-src/xcvu3p.device" )) };
	auto phys{ TimerVal(PhysGZ(phys_file)) };

	std::vector<cl_context_properties> context_properties;

	ocl::platform::each([&](uint64_t platform_idx, ocl::platform platform) {
		platform.each_device<CL_DEVICE_TYPE_GPU>([&](uint64_t device_idx, ocl::device device) {
			context_properties.emplace_back(CL_CONTEXT_PLATFORM);
			context_properties.emplace_back(std::bit_cast<cl_context_properties>(platform.platform));
		});
	});
	context_properties.emplace_back(0);

	auto ocltr{ TimerVal(OCL_Tile_Router::make(dev.root, phys.root, context_properties)) };
	TimerVal(ocltr.do_all()).value();
	puts("complete\n");
}