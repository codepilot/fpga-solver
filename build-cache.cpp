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

#include "constexpr_intrinsics.h"
#include "constexpr_siphash.h"
#include "Trivial_Span.h"

#include <Windows.h>
#include "MemoryMappedFile.h"
#include "cached_node_lookup.h"

#include <zlib.h>

int main(int argc, char* argv[]) {
	std::vector<std::string> args;

	for (int i{}; i != argc; i++) {
		args.emplace_back(argv[i]);
	}
	if (args.size() == 1) {
		args.emplace_back("C:/Users/root/Desktop/fpga-solver/build/benchmarks/xcvu3p.device");
		args.emplace_back("C:/Users/root/Desktop/fpga-solver/build/cache/xcvu3p.device.indirect.bin");
		args.emplace_back("C:/Users/root/Desktop/fpga-solver/build/cache/xcvu3p.device.direct.bin");
		args.emplace_back("C:/Users/root/Desktop/fpga-solver/build/cache/xcvu3p.device.direct.data.bin");
	}
	if (args.size() != 5) {
		return 1;
	}
	{
		int32_t arg_index{};
		for (auto&& arg : args) {
			std::print("[{}]{} ", arg_index, arg);
			arg_index++;
		}
		std::print("\n");
	}

	MemoryMappedFile mmf2{ args.at(2), 65536 };
	MemoryMappedFile mmf3{ args.at(3), 65536 };
	MemoryMappedFile mmf4{ args.at(4), 65536 };


	return 0;
}