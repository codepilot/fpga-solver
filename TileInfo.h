#pragma once

#include "PIP_Index.h"
#include "stub_router.h"
#include <format>
#include <mutex>

class TileInfo {
public:
  std::string_view name;
  Tile_Index tile_idx;
  tile_reader tile;
  tile_type_reader tile_type;
  std::set<Tile_Index> in_tiles, out_tiles, reachable_tiles;
  std::set<uint32_t> unhandled_in_nets;
  std::vector<Stub_Router*> unhandled_out_nets;
  std::vector<Stub_Router*> handling_out_nets;
  void handle_out_nets() {
	  std::scoped_lock lock(protector);
	  handling_out_nets = std::move(unhandled_out_nets);
  }
  static std::string get_tile_path_str(std::span<TileInfo, tile_count> ti, std::shared_ptr<Stub_Router> sr) {
	  std::string ret;
	  for (auto& tpi : sr->tile_path) {
		  ret += std::format("{} ", ti[tpi._value].name);
	  }
	  return ret;
  }
  std::mutex protector;
  void append_unhandled_out(Stub_Router* ustub) {
	  std::scoped_lock lock(protector);
	  unhandled_out_nets.emplace_back(ustub);
  }

};
