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

#include <zlib/zlib.h>

int main(int argc, char* argv[]) {
	std::vector<std::string_view> args;
	std::vector<std::wstring> wargs;
	for (int i{}; i != argc; i++) {
		args.emplace_back(argv[i]);
	}
	if (args.size() != 5) return 1;
	for (auto&& arg : args) {
		wargs.emplace_back(std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(std::string{ arg }));
	}
	{
		int32_t arg_index{};
		for (auto&& arg : args) {
			std::print("[{}]{} ", arg_index, arg);
			arg_index++;
		}
		std::print("\n");
	}

	MemoryMappedFile mmf2{ wargs.at(2), 65536 };
	MemoryMappedFile mmf3{ wargs.at(3), 65536 };
	MemoryMappedFile mmf4{ wargs.at(4), 65536 };


	return 0;
}