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

	auto dev{ TimerVal(DevFlat("_deps/device-file-src/xcvu3p.device" )) };
	auto phys{ TimerVal(PhysGZ("_deps/benchmark-files-src/boom_soc_unrouted.phys")) };
	auto ocltr{ TimerVal(OCL_Tile_Router::make(dev.root, phys.root)) };
	TimerVal(ocltr.do_all()).value();
}