#pragma once

#include <cstdint>
#include "interchange_types.h"

class Stub_Router {
public:
  uint32_t net_idx;
  branch_reader stub;
  std::vector<uint32_t> nodes;
  std::vector<Tile_Index> source_tiles;
  std::set<Tile_Index> avoid_tiles;
  std::vector<Tile_Index> tile_path;
  double_t current_distance;
};