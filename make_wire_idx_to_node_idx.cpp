#include "make_wire_idx_to_node_idx.h"

int main(int argc, char* argv[]) {
	std::vector<std::string> args;
	for (auto&& arg : std::span<char*>(argv, static_cast<size_t>(argc))) args.emplace_back(arg);

	const auto fn_bin{ (args.size() >= 2) ? args.at(1) : "lib_wire_idx_to_node_idx.bin" };
	const auto fn_cpp{ (args.size() >= 3) ? args.at(2) : "lib_wire_idx_to_node_idx.cpp" };
	const auto fn_h{ (args.size() >= 4) ? args.at(3) : "lib_wire_idx_to_node_idx.h" };

	TimerVal(Wire_Idx_to_Node_Idx::make(fn_bin, xcvu3p::root));
	std::ofstream f_cpp(fn_cpp, std::ios::binary);
	f_cpp << std::format(R"(#pragma once
#include "{}"
)", fn_h);


	std::ofstream f_h(fn_h, std::ios::binary);
	f_h << std::format(R"(#pragma once
#include "lib_dev_flat.h"
#include "wire_idx_to_node_idx.h"

namespace xcvu3p {{
static inline const MemoryMappedFile mmf_v_wire_idx_to_node_idx{{ "{}" }};
static inline const Wire_Idx_to_Node_Idx wire_idx_to_node_idx{{ mmf_v_wire_idx_to_node_idx.get_span<uint32_t>() }};
}};

)", fn_bin);

	return 0;
}