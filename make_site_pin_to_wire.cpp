#include "make_site_pin_to_wire.h"

int main(int argc, char* argv[]) {
	std::vector<std::string> args;
	for (auto&& arg : std::span<char*>(argv, static_cast<size_t>(argc))) args.emplace_back(arg);

	const auto fn_bin{ (args.size() >= 2) ? args.at(1) : "lib_site_pin_to_wire.bin" };
	const auto fn_cpp{ (args.size() >= 3) ? args.at(2) : "lib_site_pin_to_wire.cpp" };
	const auto fn_h{ (args.size() >= 4) ? args.at(3) : "lib_site_pin_to_wire.h" };

	TimerVal(Site_Pin_to_Wire::make(fn_bin, xcvu3p::root));

	std::ofstream f_cpp(fn_cpp, std::ios::binary);
	f_cpp << std::format(R"(
#include "{}"
)", fn_h);


	std::ofstream f_h(fn_h, std::ios::binary);
	f_h << std::format(R"(#pragma once
#include "lib_dev_flat.h"
#include "site_pin_to_wire.h"

namespace xcvu3p {{
static inline const MemoryMappedFile mmf_v_site_pin_to_wire{{ "{}" }};
static inline const Site_Pin_to_Wire site_pin_to_wire{{ mmf_v_site_pin_to_wire.get_span<uint64_t>() }};
}};

)", fn_bin);

	return 0;
}