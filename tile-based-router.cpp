#ifndef _DEBUG
#ifndef NDEBUG
#define _DEBUG
#endif
#endif

#include "tile-based-router.h"

inline static const std::string default_file{ "logicnets_jscl" };

#include "vk.hpp"

void enum_node_wire_tile_tw() {
	auto a_node_counts{ std::make_unique<std::array<uint16_t, xcvu3p::node_count>>() };
	auto a_node_offsets{ std::make_unique<std::array<int32_t, xcvu3p::node_count>>() };
	auto a_node_tileIdx{ std::make_unique<std::array<uint32_t, xcvu3p::wire_count>>() };
	auto a_node_tilePos{ std::make_unique<std::array<std::array<uint16_t, 2>, xcvu3p::wire_count>>() };
	auto a_tileStrIdx_to_tileIdx{ std::make_unique<std::array<uint32_t, xcvu3p::str_count>>() };
	auto a_tileTypeWires{ std::make_unique<std::array<std::array<uint32_t, xcvu3p::str_count>, xcvu3p::tileType_count>>() };
	auto a_tile_wire_to_node{ std::make_unique<std::array<std::vector<uint32_t>, xcvu3p::tile_count>>() };

	auto s_node_tileIdx{ std::span(*a_node_tileIdx.get()) };
	auto s_node_tilePos{ std::span(*a_node_tilePos.get()) };
	auto s_node_counts{ std::span(*a_node_counts.get()) };
	auto s_node_offsets{ std::span(*a_node_offsets.get()) };
	auto s_tileStrIdx_to_tileIdx{ std::span(*a_tileStrIdx_to_tileIdx.get()) };
	std::vector<std::span<uint32_t, xcvu3p::str_count>> vs_tileTypeWires;
	vs_tileTypeWires.reserve(xcvu3p::tileType_count);
	std::ranges::copy(*a_tileTypeWires.get(), std::back_inserter(vs_tileTypeWires));
	auto ss_tileTypeWires{ std::span(vs_tileTypeWires) };

	std::ranges::fill(s_tileStrIdx_to_tileIdx, UINT32_MAX);
	std::ranges::for_each(vs_tileTypeWires, [](std::span<uint32_t, xcvu3p::str_count> ttwires) { std::ranges::fill(ttwires, UINT32_MAX); });

	auto s_tile_wire_to_node{ std::span(*a_tile_wire_to_node.get()) };

	for (uint32_t tileIdx = 0; tileIdx < xcvu3p::tile_count; tileIdx++) {
		auto tile{ xcvu3p::tiles[tileIdx] };
		auto tileTypeIdx{ tile.getType() };
		auto tileType{ xcvu3p::tileTypes[tileTypeIdx] };

		s_tileStrIdx_to_tileIdx[xcvu3p::tiles[tileIdx].getName()] = tileIdx;
		s_tile_wire_to_node[tileIdx].resize(tileType.getWires().size(), UINT32_MAX);

	}

	for (uint32_t tileTypeIdx = 0; tileTypeIdx < xcvu3p::tileType_count; tileTypeIdx++) {
		const auto tileType{ xcvu3p::tileTypes[tileTypeIdx] };
		const auto tileWires{ tileType.getWires() };
		const auto s_tileTypeWires{ ss_tileTypeWires[tileTypeIdx] };
		for (uint16_t tileWireIdx = 0; tileWireIdx < tileWires.size(); tileWireIdx++) {
			const auto tileWireStrIdx{ tileWires[tileWireIdx] };
			s_tileTypeWires[tileWireStrIdx] = tileWireIdx;
		}
	}

	int32_t nodeOffset{};
	each<uint32_t>(xcvu3p::nodes, [&](uint32_t nodeIdx, node_reader node) {
		s_node_offsets[nodeIdx] = nodeOffset;
		nodeOffset += node.getWires().size();
	});

	jthread_each<uint32_t>(xcvu3p::nodes, [&](uint32_t nodeIdx, node_reader node) {
		auto nodeWires{ node.getWires() };
		s_node_counts[nodeIdx] = nodeWires.size();
		each<uint32_t>(nodeWires, [&](uint32_t nodeWireIdx, uint32_t wireIdx) {
			auto wire{ xcvu3p::wires[wireIdx] };
			auto wireNameStrIdx{ wire.getWire() };
			auto wireTileNameStrIdx{ wire.getTile() };
			auto tileIdx{ s_tileStrIdx_to_tileIdx[wireTileNameStrIdx] };
			auto tile{ xcvu3p::tiles[tileIdx] };
			auto tileTypeIdx{ tile.getType() };
			auto tileWireIdx{ ss_tileTypeWires[tileTypeIdx][wireNameStrIdx] };
			s_tile_wire_to_node[tileIdx][tileWireIdx] = nodeIdx;
			auto node_offset{ s_node_offsets[nodeIdx] };
			s_node_tileIdx[node_offset + nodeWireIdx] = tileIdx;
			s_node_tilePos[node_offset + nodeWireIdx] = std::array<uint16_t, 2>{tile.getCol(), tile.getRow()};
		});
	});

	std::cout << std::format("nodeOffset:{} wire_count:{} diff: {}\n", nodeOffset, xcvu3p::wire_count, xcvu3p::wire_count - nodeOffset);
}

int main(int argc, char* argv[]) {

	enum_node_wire_tile_tw();
	// vk_route::init();

#if 0
	std::cout << std::format("device: {}\n", xcvu3p::name);
	std::vector<std::string> args;
	for (auto&& arg : std::span<char*>(argv, static_cast<size_t>(argc))) args.emplace_back(arg);

	auto src_phys_file{ (args.size() >= 2) ? args.at(1) : ("_deps/benchmark-files-src/" + default_file + "_unrouted.phys") };
	auto dst_phys_file{ (args.size() >= 3) ? args.at(2) : ("_deps/benchmark-files-build/" + default_file + ".phys") };
	auto physGZV{ TimerVal(PhysGZV (src_phys_file)) };

	Tile_Based_Router tbr{ physGZV.root };
	// auto tbr{ TimerVal(Tile_Based_Router::make(physGZV.root)) };
	TimerVal(tbr.block_all_resources());
	TimerVal(tbr.route_step());
#endif

	return 0;
}