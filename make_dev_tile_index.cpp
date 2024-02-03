#include "make_dev_tile_index.h"

bool make_tile_index(const std::string fn_bin) {
	MemoryMappedFile mmf_bin{fn_bin, sizeof(std::array<uint32_t, xcvu3p::str_count>)};
	auto s_tile_str_idx_to_tile_idx(mmf_bin.get_span<uint32_t, xcvu3p::str_count>());
	std::ranges::fill(s_tile_str_idx_to_tile_idx, UINT32_MAX);
	each<xcvu3p::str_index, tile_list_reader, tile_reader>(xcvu3p::tiles, [&] (auto tile_index, auto tile) {
		s_tile_str_idx_to_tile_idx[tile.getName()] = static_cast<uint32_t>(tile_index);
	});
	return true;
}

bool make_site_index(const std::string fn_bin) {
	MemoryMappedFile mmf_bin{fn_bin, sizeof(std::array<site_info, xcvu3p::str_count>)};
	auto s_site_info(mmf_bin.get_span<site_info, xcvu3p::str_count>());
	std::ranges::fill(s_site_info, std::bit_cast<site_info>(UINT32_MAX));
	uint32_t max_site_count{};
	uint32_t max_site_index{};
	uint32_t max_site_type{};
	xcvu3p::each_tile(xcvu3p::tiles, [&] (auto tile_idx, tile_reader tile) {
		auto sites{ tile.getSites() };
		max_site_count = std::max(max_site_count, sites.size());
		each<uint32_t, decltype(sites), site_reader>(sites, [&](auto site_index, site_reader site) {
			max_site_index = std::max(max_site_index, site_index);
			max_site_type = std::max(max_site_type, site.getType());
			s_site_info[tile.getName()] = site_info{ .tile_index{tile_idx}, .site_index{site_index} };
		});
	});
	std::cout << std::format("max_site_count: {}\n", max_site_count);
	std::cout << std::format("max_site_index: {}\n", max_site_index);
	std::cout << std::format("max_site_type: {}\n", max_site_type);
	return true;
}


int main(int argc, char* argv[]) {
  	std::vector<std::string> args;
	for (auto &&arg: std::span<char*>(argv, static_cast<size_t>(argc))) args.emplace_back(arg);

	std::cout << std::format("device name: {}\n", xcvu3p::root.getName().cStr());

	const auto fn_bin{ (args.size() >= 2) ? args.at(1) : "lib_dev_tile_index.bin" };
	const auto fn_cpp{ (args.size() >= 3) ? args.at(2) : "lib_dev_tile_index.cpp" };
	const auto fn_h{ (args.size() >= 4) ? args.at(3) : "lib_dev_tile_index.h" };

	TimerVal(make_tile_index(fn_bin));
	TimerVal(make_site_index("site_info.bin"));
	
	std::ofstream f_cpp(fn_cpp, std::ios::binary);
	std::ofstream f_h(fn_h, std::ios::binary);
	f_h << R"(#pragma once
#include "lib_dev_flat.h"
)";

  return 0;
}