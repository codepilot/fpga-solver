#include "make_inverse_wires.h"

int main(int argc, char* argv[]) {
  	std::vector<std::string> args;
	for (auto &&arg: std::span<char*>(argv, static_cast<size_t>(argc))) args.emplace_back(arg);

	const auto fn_bin{ (args.size() >= 2) ? args.at(1) : "lib_inverse_wires.bin" };
	const auto fn_cpp{ (args.size() >= 3) ? args.at(2) : "lib_inverse_wires.cpp" };
	const auto fn_h{ (args.size() >= 4) ? args.at(3) : "lib_inverse_wires.h" };

	Inverse_Wires::make(fn_bin, xcvu3p::wires, xcvu3p::strs, xcvu3p::tiles, xcvu3p:: tileTypes);
	// TimerVal(make_tile_index(fn_bin));
	// TimerVal(make_site_index("site_info.bin"));
	
	std::ofstream f_cpp(fn_cpp, std::ios::binary);
	f_cpp << std::format(R"(#pragma once
#include "{}"
)", fn_h);


	std::ofstream f_h(fn_h, std::ios::binary);
	f_h << std::format(R"(#pragma once
#include "lib_dev_flat.h"
#include "inverse_wires.h"

namespace xcvu3p {{
static inline const MemoryMappedFile mmf_v_inverse_wires{{ "{}" }};
static inline const Inverse_Wires inverse_wires{{ mmf_v_inverse_wires.get_span<uint64_t>() }};
}};

)", fn_bin);

  return 0;
}