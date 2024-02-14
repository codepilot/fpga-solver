#ifndef _DEBUG
#ifndef NDEBUG
#define _DEBUG
#endif
#endif

#include "tile-based-router.h"

inline static const std::string default_file{ "logicnets_jscl" };

#include "vk.hpp"

int main(int argc, char* argv[]) {

	vk_route::init();

#if 0
	std::cout << std::format("device: {}\n", xcvu3p::name);
	std::vector<std::string> args;
	for (auto&& arg : std::span<char*>(argv, static_cast<size_t>(argc))) args.emplace_back(arg);

	auto src_phys_file{ (args.size() >= 2) ? args.at(1) : ("_deps/benchmark-files-src/" + default_file + "_unrouted.phys") };
	auto dst_phys_file{ (args.size() >= 3) ? args.at(2) : ("_deps/benchmark-files-build/" + default_file + ".phys") };
	auto physGZV{ TimerVal(PhysGZV (src_phys_file)) };

	Tile_Based_Router tbr{ physGZV.root };
	// auto tbr{ TimerVal(Tile_Based_Router::make(physGZV.root)) };
	// TimerVal(tbr.block_all_resources());
	// TimerVal(tbr.route_step());
#endif

	return 0;
}