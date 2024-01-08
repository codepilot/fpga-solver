#pragma once

#include <unordered_map>
#include <vector>
#include <cinttypes>
#include "PIP_Index.h"

class String_Building_Group {
public:
	::capnp::List< ::capnp::Text, ::capnp::Kind::BLOB>::Reader devStrs;
	::capnp::List< ::capnp::Text, ::capnp::Kind::BLOB>::Reader physStrs;
	decltype(physStrs.size()) physStrs_size;
	decltype(devStrs.size()) devStrs_size;
	std::unordered_map<String_Index, String_Index> dev_strIdx_to_phys_strIdx;
	std::vector<String_Index> phys_stridx_to_dev_stridx;
	std::vector<String_Index> extra_dev_strIdx;
	std::vector<uint32_t> dev_tile_strIndex_to_tile;
	::capnp::List< ::DeviceResources::Device::Tile, ::capnp::Kind::STRUCT>::Reader tiles;

	String_Building_Group(decltype(devStrs) devStrs, decltype(physStrs) physStrs, decltype(tiles) tiles) :
		devStrs{ devStrs },
		physStrs{ physStrs },
		physStrs_size{ physStrs.size() },
		devStrs_size{devStrs.size()},
		dev_strIdx_to_phys_strIdx{},
		phys_stridx_to_dev_stridx{std::vector<String_Index>(static_cast<size_t>(physStrs_size), String_Index{UINT32_MAX})},
		extra_dev_strIdx{},
		dev_tile_strIndex_to_tile{ std::vector<uint32_t>(static_cast<size_t>(devStrs.size()), UINT32_MAX) },
		tiles{ tiles } {

		for (uint32_t tile_idx{}; tile_idx < tiles.size(); tile_idx++) {
			auto tile{ tiles[tile_idx] };
			dev_tile_strIndex_to_tile[tile.getName()] = tile_idx;
		}

		std::unordered_map<std::string_view, String_Index> dev_strmap;
		dev_strmap.reserve(devStrs.size());
		for (uint32_t dev_strIdx{}; dev_strIdx < devStrs.size(); dev_strIdx++) {
			auto dev_str{ devStrs[dev_strIdx] };
			dev_strmap.insert({ dev_str.cStr(), String_Index{dev_strIdx} });
		}

		for (uint32_t phys_strIdx{}; phys_strIdx < physStrs.size(); phys_strIdx++) {
			std::string_view phys_str{ physStrs[phys_strIdx].cStr() };
			if (dev_strmap.contains(phys_str)) {
				auto dev_strIdx{ dev_strmap.at(phys_str) };
				phys_stridx_to_dev_stridx[phys_strIdx] = dev_strIdx;
				dev_strIdx_to_phys_strIdx.insert({ dev_strIdx, String_Index{phys_strIdx} });
			}
		}
	}

	String_Index get_phys_strIdx_from_dev_strIdx(String_Index dev_strIdx) {
		if (dev_strIdx_to_phys_strIdx.contains(dev_strIdx)) {
			return dev_strIdx_to_phys_strIdx.at(dev_strIdx);
		}
		String_Index ret{ static_cast<uint32_t>(physStrs_size + extra_dev_strIdx.size()) };
		extra_dev_strIdx.emplace_back(dev_strIdx);
		phys_stridx_to_dev_stridx.emplace_back(dev_strIdx);
		dev_strIdx_to_phys_strIdx.insert({ dev_strIdx, ret });
		return ret;
	}

};
