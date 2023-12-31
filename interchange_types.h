#pragma once

#include <unordered_map>
#include <queue>

#include "DeviceResources.capnp.h"
#include "LogicalNetlist.capnp.h"
#include "PhysicalNetlist.capnp.h"

#include "PIP_Index.h"
#include <unordered_map>
#include <span>
#include <bit>
#include <bitset>

using branch = ::PhysicalNetlist::PhysNetlist::RouteBranch;
using branch_reader = ::PhysicalNetlist::PhysNetlist::RouteBranch::Reader;
using branch_builder = ::PhysicalNetlist::PhysNetlist::RouteBranch::Builder;
using branch_list = ::capnp::List<branch, ::capnp::Kind::STRUCT>;
using branch_list_builder = branch_list::Builder;
using branch_list_reader = branch_list::Reader;
using tile_reader = DeviceResources::Device::Tile::Reader;
// using branch_builder_span = std::span<branch_builder>;
using branch_builder_map = std::unordered_map<uint32_t, branch_builder>;
using u32_span = std::span<uint32_t>;
using branch_reader_node_map = std::unordered_map<uint32_t, branch_reader>;
using branch_reader_pip_map = std::unordered_map<PIP_Index, branch_reader>;
using tile_reader_map = std::unordered_map<uint32_t, std::vector<tile_reader>>;

