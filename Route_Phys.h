#pragma once

#include "interchange_types.h"
#include "InterchangeGZ.h"
#include "RenumberedWires.h"
#include "NodeStorage.h"
#include "String_Building_Group.h"
#include "RouteStorage.h"
#include <thread>
#include "PIP_Index.h"
#include "stub_router.h"
#include <barrier>
#include "each.h"
#include "Timer.h"

class Counter {
public:
	uint64_t value{};
	constexpr void increment() noexcept { value++; }
};

class InverseWires {
public:
	inline static constexpr uint64_t wire_count{ 83282368ull };
	inline static constexpr uint64_t str_count{ 467843ull };

	static std::vector<uint64_t> make(wire_list_reader wires) {
		std::priority_queue<uint64_t, std::vector<uint64_t>> q;
		each(wires, [&](uint64_t wire_idx, wire_reader &wire) {
			q.push(((static_cast<uint64_t>(wire.getTile()) * str_count) + static_cast<uint64_t>(wire.getWire())) * wire_count);
		});
	}

	InverseWires(wire_list_reader wires) {
	}
};

#include "wire_tile_node.h"
#include "site_pin_nodes.h"
#include "node_tile_pip.h"
#include "tile_pip_node.h"
#include "site_pin_wire.h"
#include "tile_tile_wire_pip.h"
#include "TileInfo.h"
#include "route_tiles.h"
#include "Tile_Info.h"

class Route_Phys {

public:

	Counter fully_routed, skip_route, failed_route, total_attempts;
	DevFlat dev{ "_deps/device-file-build/xcvu3p.device" };
	PhysGZ phys{ "_deps/benchmark-files-src/boom_med_pb_unrouted.phys" };
//	PhysGZ phys{ "_deps/benchmark-files-src/boom_soc_unrouted.phys" };
//  PhysGZ phys{ "_deps/benchmark-files-src/corescore_500_pb_unrouted.phys" };
//  PhysGZ phys{ "_deps/benchmark-files-src/corescore_500_unrouted.phys" };
//  PhysGZ phys{ "_deps/benchmark-files-src/corundum_25g_unrouted.phys" };
//	PhysGZ phys{ "_deps/benchmark-files-src/finn_radioml_unrouted.phys" };
//  PhysGZ phys{ "_deps/benchmark-files-src/ispd16_example2_unrouted.phys" };
//  PhysGZ phys{ "_deps/benchmark-files-src/koios_dla_like_large_unrouted.phys" };
// 	PhysGZ phys{ "_deps/benchmark-files-src/logicnets_jscl_unrouted.phys" };
//  PhysGZ phys{ "_deps/benchmark-files-src/mlcad_d181_lefttwo3rds_unrouted.phys" };
//	PhysGZ phys{ "_deps/benchmark-files-src/rosetta_fd_unrouted.phys" };
//  PhysGZ phys{ "_deps/benchmark-files-src/vtr_lu64peeng_unrouted.phys" };
//  PhysGZ phys{ "_deps/benchmark-files-src/vtr_mcml_unrouted.phys" };

	decltype(dev.root) devRoot{ dev.root };
	decltype(phys.root) physRoot{ phys.root };
	decltype(devRoot.getStrList()) devStrs{ devRoot.getStrList() };
	decltype(devRoot.getTileList()) tiles{ devRoot.getTileList() };
	decltype(devRoot.getTileTypeList()) tile_types{ devRoot.getTileTypeList() };
	decltype(devRoot.getSiteTypeList()) siteTypes{ devRoot.getSiteTypeList() };
	decltype(physRoot.getStrList()) physStrs{ physRoot.getStrList() };
	decltype(devRoot.getWires()) wires{ devRoot.getWires() };
	decltype(devRoot.getWireTypes()) wireTypes{ devRoot.getWireTypes() };
	decltype(devRoot.getNodes()) nodes{ devRoot.getNodes() };

	Search_Wire_Tile_Wire search_wire_tile_wire;
	Search_Wire_Tile_Node search_wire_tile_node;
	Search_Node_Tile_Pip search_node_tile_pip;
	Search_Tile_Pip_Node search_tile_pip_node;
	Search_Site_Pin_Wire search_site_pin_wire;
	Search_Site_Pin_Node search_site_pin_node;
	Search_Tile_Tile_Wire_Pip search_tile_tile_wire_pip;

	const RenumberedWires rw{ RenumberedWires::load() };
	NodeStorage ns{ rw.alt_nodes.size(), rw };
	String_Building_Group sbg;
	// ::capnp::MallocMessageBuilder message{ 0x1FFFFFFFu, ::capnp::AllocationStrategy::FIXED_SIZE };
	::capnp::MallocMessageBuilder message{ };
	PhysicalNetlist::PhysNetlist::Builder physBuilder{ message.initRoot<PhysicalNetlist::PhysNetlist>() };
	std::vector<std::array<uint16_t, 2>> unrouted_locations;
	std::span<uint32_t> routed_indices;
	std::span<uint32_t> unrouted_indices;
	std::span<uint32_t> stubs_indices;
	std::atomic<uint32_t> routed_index_count{};
	std::atomic<uint32_t> unrouted_index_count{};
	std::atomic<uint32_t> stubs_index_count{};
	std::jthread jt;

	Route_Phys() :
		sbg{ devStrs , physStrs, devRoot.getTileList() }
	{
		puts(std::format("Route_Phys() rw.node_count: {}, rw.pip_count: {}", rw.alt_nodes.size(), rw.alt_pips.size()).c_str());

#if 0
		for (auto &&tile: tiles) {
			unrouted_locations.emplace_back(std::array<uint16_t, 2>{tile.getCol(), tile.getRow()});
		}
#else
		for (uint16_t row{}; row < 311; row++) {
			for (uint16_t col{}; col < 670; col++) {
				unrouted_locations.emplace_back(std::array<uint16_t, 2>{col, row});
			}
		}
#endif
		puts(std::format("Route_Phys() finish").c_str());
	}

	void start_routing(std::span<uint32_t> routed_indices_mapping, std::span<uint32_t> unrouted_indices_mapping, std::span<uint32_t> stubs_indices_mapping) {
		routed_indices = routed_indices_mapping;
		unrouted_indices = unrouted_indices_mapping;
		stubs_indices = stubs_indices_mapping;

		jt = std::jthread{ [this](std::stop_token stoken) {
			route(stoken);
			// tile_based_routing();
		} };
	}

	void block_site_pin(uint32_t net_idx, ::PhysicalNetlist::PhysNetlist::PhysSitePin::Reader sitePin) {
		String_Index ps_source_site{ sitePin.getSite() };
		String_Index ps_source_pin{ sitePin.getPin() };
		// OutputDebugStringA(std::format("block_site_pin({}, {})\n", strList[ps_source_site].cStr(), strList[ps_source_pin].cStr()).c_str());
		String_Index ds_source_site{ sbg.phys_stridx_to_dev_stridx.at(ps_source_site._strIdx) };
		String_Index ds_source_pin{ sbg.phys_stridx_to_dev_stridx.at(ps_source_pin._strIdx) };
		// OutputDebugStringA(std::format("block_site_pin({}, {})\n", dev.strList[ds_source_site].cStr(), dev.strList[ds_source_pin].cStr()).c_str());
		
		auto source_wire_idx{ rw.find_site_pin_wire(ds_source_site, ds_source_pin) };
		if (source_wire_idx != UINT32_MAX) {
			auto source_node_idx{ rw.alt_wire_to_node[source_wire_idx] };
			ns.store_node(net_idx, source_node_idx);
		}
	}

	void block_pip(uint32_t net_idx, ::PhysicalNetlist::PhysNetlist::PhysPIP::Reader pip) {
		auto ps_tile{ pip.getTile() };
		auto ps_wire0{ pip.getWire0() };
		auto ps_wire1{ pip.getWire1() };

		auto ds_tile{ sbg.phys_stridx_to_dev_stridx.at(ps_tile) };
		auto ds_wire0{ sbg.phys_stridx_to_dev_stridx.at(ps_wire0) };
		auto ds_wire1{ sbg.phys_stridx_to_dev_stridx.at(ps_wire1) };

		auto wire0_idx{ rw.find_wire(ds_tile, ds_wire0) };
		auto wire1_idx{ rw.find_wire(ds_tile, ds_wire1) };


		auto node0_idx{ rw.alt_wire_to_node[wire0_idx] };
		auto node1_idx{ rw.alt_wire_to_node[wire1_idx] };

		ns.store_node(net_idx, node0_idx);
		ns.store_node(net_idx, node1_idx);
	}

	void block_source_resource(uint32_t net_idx, PhysicalNetlist::PhysNetlist::RouteBranch::Reader branch) {
		auto rs{ branch.getRouteSegment() };
		switch (rs.which()) {
		case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::Which::BEL_PIN: {
			auto belPin{ rs.getBelPin() };
			// block_bel_pin(net_idx, belPin);
			break;
		}
		case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::Which::SITE_PIN: {
			auto sitePin{ rs.getSitePin() };
			block_site_pin(net_idx, sitePin);
			break;
		}
		case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::Which::PIP: {
			auto pip{ rs.getPip() };
			block_pip(net_idx, pip);
			break;
		}
		case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::Which::SITE_P_I_P: {
			auto sitePip{ rs.getSitePIP() };
			// block_site_pip(net_idx, sitePip);
			break;
		}
		default:
			abort();
		}
		for (auto&& sub_branch : branch.getBranches()) {
			block_source_resource(net_idx, sub_branch);
		}
	}

	void block_resources(uint32_t net_idx, PhysicalNetlist::PhysNetlist::PhysNet::Reader physNet) {
		for (auto&& src_branch : physNet.getSources()) {
			block_source_resource(net_idx, src_branch);
		}
		for (auto&& stub_branch : physNet.getStubs()) {
			block_source_resource(net_idx, stub_branch);
		}

	}

	std::optional<branch_builder> branch_site_pin(uint32_t nameIdx, branch_builder branch) {
		auto name{ physStrs[nameIdx] };
		auto branch_rs{ branch.getRouteSegment() };
		auto branch_branches{ branch.getBranches() };
		auto branch_rs_which{ branch_rs.which() };
		// OutputDebugStringA(std::format("{} branches({}) which {}\n", name.cStr(), branch_branches.size(), static_cast<uint16_t>(branch_rs_which)).c_str());
		switch (branch_rs_which) {
			case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::BEL_PIN: {
				if (!branch_branches.size()) return std::nullopt;
				for (auto&& subbranch : branch_branches) {
					auto ret{ branch_site_pin(nameIdx, subbranch) };
					if (ret.has_value()) return ret;
				}
				return std::nullopt;
			}
			case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::SITE_PIN: {
				if (!branch_branches.size()) return branch;
				for (auto&& subbranch : branch_branches) {
					auto ret{ branch_site_pin(nameIdx, subbranch) };
					if (ret.has_value()) return ret;
				}
				return std::nullopt;
			}
			case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::PIP: {
				puts("PIP");
				abort();
				break;
			}
			case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::SITE_P_I_P: {
				if(!branch_branches.size()) return std::nullopt;
				for (auto&& subbranch : branch_branches) {
					auto ret{ branch_site_pin(nameIdx, subbranch) };
					if (ret.has_value()) return ret;
				}
				return std::nullopt;

			}
			default: {
				puts("unreachable");
				abort();
			}
		}
		return std::nullopt;
	}

	uint32_t get_tile_idx_from_wire_idx(uint32_t wire_idx) const {
		auto tile_str{ rw.get_wire_tile_str(wire_idx) };
		auto tile_idx{ sbg.dev_tile_strIndex_to_tile.at(tile_str._strIdx) };
		return tile_idx;
	}

	uint32_t get_tile_idx_from_site_pin(::PhysicalNetlist::PhysNetlist::PhysSitePin::Builder sitePin) const {
		auto wire_idx{ rw.find_site_pin_wire(sbg.phys_stridx_to_dev_stridx.at(sitePin.getSite()), sbg.phys_stridx_to_dev_stridx.at(sitePin.getPin())) };
		return get_tile_idx_from_wire_idx(wire_idx);
	}

	void draw_sources(uint32_t nameIdx, branch_list_builder branches, uint32_t previous_tile = UINT32_MAX) {
		for (auto&& branch : branches) {
			auto branch_rs{ branch.getRouteSegment() };
			auto branch_rs_which{ branch_rs.which() };
			auto branch_branches{ branch.getBranches() };

			switch (branch_rs_which) {
			case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::BEL_PIN: {
				draw_sources(nameIdx, branch_branches, previous_tile);
				break;
			}
			case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::SITE_PIN: {
				auto tile_idx{ get_tile_idx_from_site_pin(branch_rs.getSitePin()) };

				if (previous_tile != UINT32_MAX && routed_index_count + 2 < routed_indices.size()) {
					routed_indices[routed_index_count++] = previous_tile;
					routed_indices[routed_index_count++] = tile_idx;
				}

				draw_sources(nameIdx, branch_branches, tile_idx);
				break;
			}
			case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::PIP: {
				auto pip{ branch_rs.getPip() };
				auto tile_idx{ sbg.dev_tile_strIndex_to_tile.at(sbg.phys_stridx_to_dev_stridx.at(pip.getTile())._strIdx) };

				if (previous_tile != UINT32_MAX && routed_index_count + 2 < routed_indices.size()) {
					routed_indices[routed_index_count++] = previous_tile;
					routed_indices[routed_index_count++] = tile_idx;
				}

				draw_sources(nameIdx, branch_branches, tile_idx);
				break;
			}
			case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::SITE_P_I_P: {
				draw_sources(nameIdx, branch_branches, previous_tile);
				break;
			}
			default: {
				puts("unreachable");
				abort();
			}
			}
		}
		// routed_indices.emplace_back(unrouted_locations.size()); routed_index_count++;
	}

	void route(std::stop_token stoken) {
		puts(std::format("route start").c_str());

		physBuilder.setPart(physRoot.getPart());
		physBuilder.setPlacements(physRoot.getPlacements());
		auto readerPhysNets{ physRoot.getPhysNets() };
		uint32_t readerPhysNets_size{ readerPhysNets.size() };
		physBuilder.initPhysNets(readerPhysNets_size);
		auto listPhysNets{ physBuilder.getPhysNets() };
		for (uint32_t net_idx{}; net_idx < readerPhysNets.size(); net_idx++) {
			block_resources(net_idx, readerPhysNets[net_idx]);
		}
		for (uint32_t n{}; n != readerPhysNets_size; n++) {
			if (!(n % 1)) {
				puts(std::format("n: {} of {}, total_attempts: {}, skip_route: {}, fully_routed: {}, failed_route: {}", n, readerPhysNets_size, total_attempts.value, skip_route.value, fully_routed.value, failed_route.value).c_str());
			}
			auto phyNetReader{ readerPhysNets[n] };

#if 0
			listPhysNets.setWithCaveats(n, phyNetReader);
#else
			auto phyNetBuilder{ listPhysNets[n] };
			phyNetBuilder.setName(phyNetReader.getName());

			auto r_sources{ phyNetReader.getSources() };
			auto r_stubs{ phyNetReader.getStubs() };

			phyNetBuilder.setSources(r_sources);
			auto b_sources{ phyNetBuilder.getSources() };

			if (r_stubs.size() == 0) {
				skip_route.increment();
			} else if (r_sources.size() > 1u && r_stubs.size() > 1u) {
				puts(std::format("{} sources:{}, stubs:{}\n", physStrs[phyNetReader.getName()].cStr(), r_sources.size(), r_stubs.size()).c_str());
			} else if (r_sources.size() == 1u && r_stubs.size() > 0u && !stoken.stop_requested()) {
#if 0
				used_nodes.clear(); used_nodes.resize(rw.alt_nodes.size(), false);
				used_pips.clear();  used_pips.resize(rw.alt_pips.size(), false);
				rw.clear_routes();
#endif

				RouteStorage rs{ rw.alt_pips.size(), ns, rw, sbg, unrouted_indices, unrouted_index_count, stubs_indices, stubs_index_count };

				if (!rs.assign_stubs(n, phyNetReader.getName(), b_sources, r_stubs)) {
					phyNetBuilder.setStubs(r_stubs);
					failed_route.increment();
				}
				else {
					fully_routed.increment();
					if(!routed_indices.empty()) draw_sources(n, b_sources);
				}
				//phyNetBuilder.initStubs(0);
			}
			else {
				phyNetBuilder.setStubs(r_stubs);
			}

			phyNetBuilder.setType(phyNetReader.getType());
			phyNetBuilder.setStubNodes(phyNetReader.getStubNodes());
#endif
		}

		physBuilder.setPhysCells(physRoot.getPhysCells());
		//phyBuilder.setStrList(phys.reader.getStrList());
		auto strListBuilder{ physBuilder.initStrList(static_cast<uint32_t>(physStrs.size() + sbg.extra_dev_strIdx.size())) };

		for (uint32_t strIdx{}; strIdx < physStrs.size(); strIdx++) {
			strListBuilder.set(strIdx, physStrs[strIdx]);
		}

		for (uint32_t extraIdx{}; extraIdx < sbg.extra_dev_strIdx.size(); extraIdx++) {
			auto dev_strIdx{ sbg.extra_dev_strIdx[extraIdx] };
			strListBuilder.set(physStrs.size() + extraIdx, devStrs[dev_strIdx._strIdx]);
		}
		physBuilder.setSiteInsts(physRoot.getSiteInsts());
		physBuilder.setProperties(physRoot.getProperties());
		physBuilder.setNullNet(physRoot.getNullNet());

		phys.write("dst_written.phy.gz", message);

		puts("route finish");

	}

	void block_phys() {
		puts("block_phys() start");
		each(physRoot.getPhysNets(), [&](uint64_t net_idx, net_reader &net) {
			block_resources(static_cast<uint32_t>(net_idx), net);
		});
		puts("block_phys() finish");
	}

	//19 bits str site, 19 bit pin, 25 bit node/branch, 1 bit leaf

	std::unique_ptr<std::array<TileInfo, tile_count>> make_ti() {
		auto upti{ std::make_unique<std::array<TileInfo, tile_count>>() };
		std::span<TileInfo, tile_count> ti{ *upti };

		puts("ti start");
		each(tiles, [&](auto tile_counter, tile_reader &tile) {
			auto tile_idx{ Tile_Index::make(tile) };
			ti[tile_idx._value].name = devStrs[tile.getName()].cStr();
			ti[tile_idx._value].tile_idx = tile_idx;
			ti[tile_idx._value].tile = tile;
			ti[tile_idx._value].tile_type = tile_types[tile.getType()];
		});
		puts("ti finish");

		return upti;
	}

	struct pip_body {
		uint32_t node0_idx : 31;
		uint32_t is_forward : 1;
		uint32_t node1_idx;
		uint32_t wire0_idx;
		uint32_t wire1_idx;
	};

	static void make_cl_pip_files() {
		{
			MemoryMappedFile mmf_v_pip_count_offset{ "pip_count_offset.bin" };
			MemoryMappedFile mmf_v_pip_tile_body{ "pip_tile_body.bin" };
			MemoryMappedFile mmf_v_pip_body{ "pip_body.bin" };
			if (mmf_v_pip_count_offset.fsize && mmf_v_pip_tile_body.fsize && mmf_v_pip_body.fsize) return;
		}

		DevFlat dev{ "_deps/device-file-build/xcvu3p.device" };
		auto devRoot{ dev.root };
		auto tiles{ devRoot.getTileList() };
		auto nodes{ devRoot.getNodes() };
		auto wires{ devRoot.getWires() };
		auto tile_types{ devRoot.getTileTypeList() };
		auto tile_bounds{ Tile_Info::get_tile_info(tiles) };
		auto devStrs{ devRoot.getStrList() };
		auto wireTypes{ devRoot.getWireTypes() };

		puts(std::format("devStrs: {}, {} bits", devStrs.size(), ceil(log2(devStrs.size()))).c_str());
		puts(std::format("tiles: {}, {} bits", tiles.size(), ceil(log2(tiles.size()))).c_str());
		puts(std::format("nodes: {}, {} bits", nodes.size(), ceil(log2(nodes.size()))).c_str());
		puts(std::format("wires: {}, {} bits", wires.size(), ceil(log2(wires.size()))).c_str());
		uint64_t group_size{ std::thread::hardware_concurrency() };
		puts(std::format("std::thread::hardware_concurrency: {}", group_size).c_str());

		std::vector<uint64_t> inverse_wires(static_cast<size_t>(wires.size()), UINT64_MAX); //wire str, tile str, wire idx log2(467843 * 467843 * 83282368) < 64
		std::vector<uint32_t> inverse_tiles(static_cast<size_t>(devStrs.size()), UINT32_MAX);
		std::vector<uint32_t> inverse_nodes(static_cast<size_t>(wires.size()), UINT32_MAX);

		const uint64_t str_count{ devStrs.size() };

		puts("inverse_wires gather");
		jthread_each(wires, [&](uint64_t wire_idx, wire_reader &wire) {
			uint64_t tile_strIdx{ wire.getTile() };
			uint64_t wire_strIdx{ wire.getWire() };
			inverse_wires[wire_idx] = (tile_strIdx * str_count + wire_strIdx) * wire_count + wire_idx;
		});
		puts("inverse_wires sort");
		std::sort(std::execution::par_unseq, inverse_wires.begin(), inverse_wires.end(), [](uint64_t a, uint64_t b) { return a < b; });

		puts("inverse_tiles");
		jthread_each(tiles, [&](uint64_t tile_idx, tile_reader &tile) {
			inverse_tiles[tile.getName()] = static_cast<uint32_t>(tile_idx);
		});

//		uint32_t max_node_size{};
		puts("inverse_nodes");
		jthread_each(nodes, [&](uint64_t node_idx, node_reader &node) {
			auto node_wires = node.getWires();
//			max_node_size = node_wires.size() > max_node_size ? node_wires.size() : max_node_size;
			for (uint32_t wire_idx : node_wires) {
				inverse_nodes[wire_idx] = static_cast<uint32_t>(node_idx);
			}
		});
//		printf("max_node_size: %u\n", max_node_size);

		std::barrier<> bar{ static_cast<ptrdiff_t>(group_size) };

		const uint64_t wire_count{ wires.size() };
		const uint64_t str_m_wire_count{ str_count * wire_count };

//		std::vector<std::array<uint32_t, 4>> verts(static_cast<size_t>(128ull * 1024ull * 1024ull), std::array<uint32_t, 4>{UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX}); // node0_idx node1_idx wire0_idx, wire1_idx
		MemoryMappedFile mmf_pip_body{ "pip_body.bin", sizeof(std::array<uint32_t, 4>) * static_cast<size_t>(128ull * 1024ull * 1024ull) };
		auto s_verts{ mmf_pip_body.get_span<pip_body>() };
		std::atomic<uint32_t> vert_idx{};

		jthread_each(tiles, [&](uint64_t tile_index, tile_reader tile)-> void {
			uint64_t tile_strIdx{ tile.getName() };
			auto tile_type{ tile_types[tile.getType()] };
			auto tile_type_wire_strs{ tile_type.getWires() };
			if (!tile_type_wire_strs.size()) return;
			auto tileIndex{ Tile_Index::make(tile) };
			auto pips{ tile_type.getPips() };
			// decltype(auto) v_tti{ v_tt.at(tileIndex._value) };

			auto tile_range{ std::ranges::equal_range(inverse_wires, tile_strIdx * str_m_wire_count, [&](uint64_t a, uint64_t b) { return (a / str_m_wire_count) < (b / str_m_wire_count); }) };
			if (!tile_range.size()) {
				puts(std::format("!tile_range {} pips {}", devStrs[tile_strIdx].cStr(), pips.size()).c_str());
				return;
			}
			// puts(std::format("*tile_range {} {} pips {}", devStrs[tile_strIdx].cStr(), tile_range.size(), pips.size()).c_str());
			for (auto&& pip : pips) {
				if (!pip.isConventional()) continue;
				uint64_t wire0_strIdx{ tile_type_wire_strs[pip.getWire0()] };
				uint64_t wire1_strIdx{ tile_type_wire_strs[pip.getWire1()] };
				auto wire0_key{ (tile_strIdx * str_count + wire0_strIdx) * wire_count };
				auto wire1_key{ (tile_strIdx * str_count + wire1_strIdx) * wire_count };
				auto wire0_idx_s{ std::ranges::equal_range(tile_range, wire0_key, [&](uint64_t a, uint64_t b) { return (a / wire_count) < (b / wire_count); }) };
				auto wire1_idx_s{ std::ranges::equal_range(tile_range, wire1_key, [&](uint64_t a, uint64_t b) { return (a / wire_count) < (b / wire_count); }) };
				if (wire0_idx_s.size() != 1) {
					puts(std::format("!inverse_wires {} {} count {}", devStrs[tile_strIdx].cStr(), devStrs[wire0_strIdx].cStr(), wire0_idx_s.size()).c_str());
					continue;
				}
				if (wire1_idx_s.size() != 1) {
					puts(std::format("!inverse_wires {} {} count {}", devStrs[tile_strIdx].cStr(), devStrs[wire1_strIdx].cStr(), wire1_idx_s.size()).c_str());
					continue;
				}
				auto wire0_idx{ static_cast<uint32_t>(wire0_idx_s[0] % wire_count) };
				auto wire1_idx{ static_cast<uint32_t>(wire1_idx_s[0] % wire_count) };
				if (wireTypes[wires[wire0_idx].getType()].getCategory() == ::DeviceResources::Device::WireCategory::GLOBAL) break;
				if (wireTypes[wires[wire1_idx].getType()].getCategory() == ::DeviceResources::Device::WireCategory::GLOBAL) break;

				auto node0_idx = inverse_nodes.at(wire0_idx);
				auto node1_idx = inverse_nodes.at(wire1_idx);

				if (node0_idx == UINT32_MAX) {
					puts(std::format("!inverse_nodes {} {}", devStrs[tile_strIdx].cStr(), devStrs[wire0_strIdx].cStr()).c_str());
					continue;
				}
				if (node1_idx == UINT32_MAX) {
					puts(std::format("!inverse_nodes {} {}", devStrs[tile_strIdx].cStr(), devStrs[wire1_strIdx].cStr()).c_str());
					continue;
				}
				s_verts[vert_idx++] = pip_body{
					.node0_idx{node0_idx},
					.is_forward{true},
					.node1_idx{node1_idx},
					.wire0_idx{wire0_idx},
					.wire1_idx{wire1_idx},
				};

				if (!pip.getDirectional()) {
					s_verts[vert_idx++] = pip_body{
						.node0_idx{node1_idx},
						.is_forward{false},
						.node1_idx{node0_idx},
						.wire0_idx{wire1_idx},
						.wire1_idx{wire0_idx},
					};
				}
			}
		});

		auto mmf_pip_body_shrunk = mmf_pip_body.shrink(sizeof(pip_body) * static_cast<size_t>(vert_idx.load()));
		auto s_verts_shrunk{ mmf_pip_body_shrunk.get_span<pip_body>() };

		MemoryMappedFile mmf_pip_tile_body{ "pip_tile_body.bin", sizeof(std::array<uint32_t, 4>) * static_cast<size_t>(vert_idx.load()) };
		auto s_pip_tile_body{ mmf_pip_tile_body.get_span<std::array<uint32_t, 4>>() };

		std::sort(std::execution::par_unseq, s_verts_shrunk.begin(), s_verts_shrunk.end(), [](const pip_body &a, const pip_body &b) {
			if (a.node0_idx < b.node0_idx) return true;
			if (a.node0_idx > b.node0_idx) return false;

			if (a.node1_idx < b.node1_idx) return true;
			if (a.node1_idx > b.node1_idx) return false;

			if (a.wire0_idx < b.wire0_idx) return true;
			if (a.wire0_idx > b.wire0_idx) return false;

			return (a.wire1_idx < b.wire1_idx);
		});

		jthread_each(s_verts_shrunk, [&](uint64_t vert_idx, pip_body& vert) {
			auto wire0 = wires[vert.wire0_idx];
			auto wire1 = wires[vert.wire1_idx];
			auto tile0_idx = inverse_tiles[wire0.getTile()];
			auto tile1_idx = inverse_tiles[wire1.getTile()];
			if (tile0_idx != tile1_idx) abort();
			auto tile0 = tiles[tile0_idx];
			s_pip_tile_body[vert_idx] = std::array<uint32_t, 4>{tile0.getCol(), tile0.getRow(), vert.node0_idx, vert.node1_idx};
		});

		MemoryMappedFile mmf_pip_count_offset{ "pip_count_offset.bin", sizeof(std::array<uint32_t, 2>) * static_cast<size_t>(nodes.size()) };
		auto s_pip_count_offset{ mmf_pip_count_offset.get_span<std::array<uint32_t, 2>>()};

		jthread_each(nodes, [&](uint64_t node_idx, node_reader& node) {
			pip_body node_key{ .node0_idx{static_cast<uint32_t>(node_idx)} };
			auto node_range{ std::span(std::ranges::equal_range(s_verts_shrunk, node_key, [](const pip_body &a, const pip_body &b)-> bool { return a.node0_idx < b.node0_idx; })) };
			uint32_t offset{ static_cast<uint32_t>(node_range.data() - s_verts_shrunk.data()) };
			uint32_t count{ static_cast<uint32_t>(node_range.size()) };
			s_pip_count_offset[node_idx] = std::array<uint32_t, 2>{count, offset};
		});
	}

	static void make_cl_files() {
		DevFlat dev{ "_deps/device-file-build/xcvu3p.device" };
		auto devRoot{ dev.root };
		auto tiles{ devRoot.getTileList() };
		auto nodes{ devRoot.getNodes() };
		auto wires{ devRoot.getWires() };
		auto tile_types{ devRoot.getTileTypeList() };
		auto tile_bounds{ Tile_Info::get_tile_info(tiles) };
		auto devStrs{ devRoot.getStrList() };
		auto wireTypes{ devRoot.getWireTypes() };

		puts(std::format("devStrs: {}, {} bits", devStrs.size(), ceil(log2(devStrs.size()))).c_str());
		puts(std::format("tiles: {}, {} bits", tiles.size(), ceil(log2(tiles.size()))).c_str());
		puts(std::format("nodes: {}, {} bits", nodes.size(), ceil(log2(nodes.size()))).c_str());
		puts(std::format("wires: {}, {} bits", wires.size(), ceil(log2(wires.size()))).c_str());
		uint64_t group_size{ std::thread::hardware_concurrency() };
		puts(std::format("std::thread::hardware_concurrency: {}", group_size).c_str());

		std::vector<uint64_t> inverse_wires(static_cast<size_t>(wires.size()), UINT64_MAX); //wire str, tile str, wire idx log2(467843 * 467843 * 83282368) < 64
		std::vector<uint32_t> inverse_tiles(static_cast<size_t>(devStrs.size()), UINT32_MAX);
		std::vector<uint32_t> inverse_nodes(static_cast<size_t>(wires.size()), UINT32_MAX);

		std::barrier<> bar{ static_cast<ptrdiff_t>(group_size) };
		std::vector<std::set<uint32_t>> v_tt(static_cast<size_t>(tiles.size()), std::set<uint32_t>{});

		{
			std::vector<std::jthread> threads;
			for (uint64_t offset{ 0 }; offset < group_size; offset++) {
				threads.emplace_back([offset, group_size, &bar, &tiles, &tile_types, &v_tt, &inverse_wires, &inverse_nodes, &inverse_tiles, &nodes, &wires, &devStrs, &wireTypes]() {

					const uint64_t str_count{ devStrs.size() };
					const uint64_t wire_count{ wires.size() };
					const uint64_t str_m_wire_count{ str_count * wire_count };

					puts("inverse_wires");
					each_n(offset, group_size, wires, [&](uint64_t wire_idx, wire_reader wire) {
						uint64_t tile_strIdx{ wire.getTile() };
						uint64_t wire_strIdx{ wire.getWire() };
						inverse_wires[wire_idx] = (tile_strIdx * str_count + wire_strIdx) * wire_count + wire_idx;
					});

					bar.arrive_and_wait();
					if (!offset) std::sort(std::execution::par_unseq, inverse_wires.begin(), inverse_wires.end(), [](uint64_t a, uint64_t b) { return a < b; });

					puts("inverse_tiles");
					each_n(offset, group_size, tiles, [&](uint64_t tile_idx, tile_reader tile) {
						inverse_tiles[tile.getName()] = static_cast<uint32_t>(tile_idx);
					});

					puts("inverse_nodes");
					each_n(offset, group_size, nodes, [&](uint64_t node_idx, node_reader node) {
						for (uint32_t wire_idx : node.getWires()) {
							inverse_nodes[wire_idx] = static_cast<uint32_t>(node_idx);
						}
					});

					bar.arrive_and_wait();

					puts("v_tt");
					each_n(offset, group_size, tiles, [&](uint64_t tile_index, tile_reader tile)-> void {
						uint64_t tile_strIdx{ tile.getName() };
						auto tile_type{ tile_types[tile.getType()] };
						auto siteTypes{ tile_type.getSiteTypes() };
						auto tile_type_wire_strs{ tile_type.getWires() };
						if (!tile_type_wire_strs.size()) return;
						auto tileIndex{ Tile_Index::make(tile) };
						auto pips{ tile_type.getPips() };
						decltype(auto) v_tti{ v_tt.at(tileIndex._value) };

						auto tile_range{ std::ranges::equal_range(inverse_wires, tile_strIdx * str_m_wire_count, [&](uint64_t a, uint64_t b) { return (a / str_m_wire_count) < (b / str_m_wire_count); }) };
						if (!tile_range.size()) {
							puts(std::format("!tile_range {} pips {}", devStrs[tile_strIdx].cStr(), pips.size()).c_str());
							return;
						}
						// puts(std::format("*tile_range {} {} pips {}", devStrs[tile_strIdx].cStr(), tile_range.size(), pips.size()).c_str());
						for (auto&& siteType : siteTypes) {
							for (auto&& wire0_strIdx : siteType.getPrimaryPinsToTileWires()) {
								auto wire0_key{ (tile_strIdx * str_count + wire0_strIdx) * wire_count };
								auto wire0_idx_s{ std::ranges::equal_range(tile_range, wire0_key, [&](uint64_t a, uint64_t b) { return (a / wire_count) < (b / wire_count); }) };
								if (wire0_idx_s.size() != 1) {
									puts(std::format("!inverse_wires {} {} count {}", devStrs[tile_strIdx].cStr(), devStrs[wire0_strIdx].cStr(), wire0_idx_s.size()).c_str());
									continue;
								}
								auto wire0_idx{ wire0_idx_s[0] % wire_count };
								if (inverse_nodes.at(wire0_idx) == UINT32_MAX) {
									puts(std::format("!inverse_nodes {} {}", devStrs[tile_strIdx].cStr(), devStrs[wire0_strIdx].cStr()).c_str());
									continue;
								}
								auto wire0_wires{ nodes[inverse_nodes.at(wire0_idx)].getWires() };
								for (auto wire_n : wire0_wires) {
									if (wireTypes[wires[wire_n].getType()].getCategory() == ::DeviceResources::Device::WireCategory::GLOBAL) break;
									auto wire_n_tile{ tiles[inverse_tiles.at(wires[wire_n].getTile())] };
									v_tti.insert(std::bit_cast<uint32_t>(std::array<uint16_t, 2>{ wire_n_tile.getCol(), wire_n_tile.getRow() }));
								}
							}
						}
						for (auto&& pip : pips) {
							if (!pip.isConventional()) continue;
							uint64_t wire0_strIdx{ tile_type_wire_strs[pip.getWire0()] };
							uint64_t wire1_strIdx{ tile_type_wire_strs[pip.getWire1()] };
							auto wire0_key{ (tile_strIdx * str_count + wire0_strIdx) * wire_count };
							auto wire1_key{ (tile_strIdx * str_count + wire1_strIdx) * wire_count };
							auto wire0_idx_s{ std::ranges::equal_range(tile_range, wire0_key, [&](uint64_t a, uint64_t b) { return (a / wire_count) < (b / wire_count); }) };
							auto wire1_idx_s{ std::ranges::equal_range(tile_range, wire1_key, [&](uint64_t a, uint64_t b) { return (a / wire_count) < (b / wire_count); }) };
							if (wire0_idx_s.size() != 1) {
								puts(std::format("!inverse_wires {} {} count {}", devStrs[tile_strIdx].cStr(), devStrs[wire0_strIdx].cStr(), wire0_idx_s.size()).c_str());
								continue;
							}
							if (wire1_idx_s.size() != 1) {
								puts(std::format("!inverse_wires {} {} count {}", devStrs[tile_strIdx].cStr(), devStrs[wire1_strIdx].cStr(), wire1_idx_s.size()).c_str());
								continue;
							}
							auto wire0_idx{ wire0_idx_s[0] % wire_count };
							auto wire1_idx{ wire1_idx_s[0] % wire_count };
							if (inverse_nodes.at(wire0_idx) == UINT32_MAX) {
								puts(std::format("!inverse_nodes {} {}", devStrs[tile_strIdx].cStr(), devStrs[wire0_strIdx].cStr()).c_str());
								continue;
							}
							if (inverse_nodes.at(wire1_idx) == UINT32_MAX) {
								puts(std::format("!inverse_nodes {} {}", devStrs[tile_strIdx].cStr(), devStrs[wire1_strIdx].cStr()).c_str());
								continue;
							}
							auto wire0_wires{ nodes[inverse_nodes.at(wire0_idx)].getWires() };
							auto wire1_wires{ nodes[inverse_nodes.at(wire1_idx)].getWires() };
							for (auto wire_n : wire0_wires) {
								if (wireTypes[wires[wire_n].getType()].getCategory() == ::DeviceResources::Device::WireCategory::GLOBAL) break;
								auto wire_n_tile{ tiles[inverse_tiles.at(wires[wire_n].getTile())] };
								v_tti.insert(std::bit_cast<uint32_t>(std::array<uint16_t, 2>{ wire_n_tile.getCol(), wire_n_tile.getRow() }));
							}
							if (!pip.getDirectional()) {
								for (auto wire_n : wire1_wires) {
									if (wireTypes[wires[wire_n].getType()].getCategory() == ::DeviceResources::Device::WireCategory::GLOBAL) break;
									auto wire_n_tile{ tiles[inverse_tiles.at(wires[wire_n].getTile())] };
									v_tti.insert(std::bit_cast<uint32_t>(std::array<uint16_t, 2>{ wire_n_tile.getCol(), wire_n_tile.getRow() }));
								}
							}
						}
					});
				});
			}
		}

		puts("v_tt_count_offset");
		std::vector<std::array<std::uint32_t, 2>> v_tt_count_offset;
		v_tt_count_offset.reserve(static_cast<size_t>(tiles.size()));
		size_t offset{};
		size_t max_count{};
		for (auto&& v_tti : v_tt) {
			v_tt_count_offset.emplace_back(std::array<uint32_t, 2>{ static_cast<uint32_t>(v_tti.size()), static_cast<uint32_t>(offset) });
			offset += v_tti.size();
			max_count = (v_tti.size() > max_count) ? v_tti.size() : max_count;
		};

		puts("v_tt_body");
		std::vector<std::array<uint16_t, 2>> v_tt_body;
		v_tt_body.reserve(offset);
		for (auto&& v_tti : v_tt) {
			for (auto&& v_ttin : v_tti) {
				v_tt_body.emplace_back(std::bit_cast<std::array<uint16_t, 2>>(v_ttin));
			}
		}
		{
			std::span<std::array<std::uint32_t, 2>> s_tt_count_offset{ v_tt_count_offset };
			MemoryMappedFile mmf_v_tt_count_offset{ "tt_count_offset.bin", s_tt_count_offset.size_bytes() };
			memcpy(mmf_v_tt_count_offset.fp, s_tt_count_offset.data(), s_tt_count_offset.size_bytes());
		}
		{
			std::span<std::array<std::uint16_t, 2>> s_tt_body{ v_tt_body };
			MemoryMappedFile mmf_v_tt_count_offset{ "tt_body.bin", s_tt_body.size_bytes() };
			memcpy(mmf_v_tt_count_offset.fp, s_tt_body.data(), s_tt_body.size_bytes());
		}
		puts(std::format("max_count: {}", max_count).c_str());
		puts(std::format("v_tt_body: {}", v_tt_body.size()).c_str());
	}

	static void make_search_files(std::string src_dev_file, std::string dst_dev_file) {
		auto dev{ TimerVal(DevGZ( src_dev_file, dst_dev_file )) };
		decltype(dev.root) devRoot{ dev.root };

		{
			auto execLambda{ Timer<"{:10} @ make_wire_tile_wire\n">() };
			Search_Wire_Tile_Wire::make_wire_tile_wire(devRoot);
		}
		Search_Wire_Tile_Wire search_wire_tile_wire;
		{
			auto execLambda{ Timer<"{:10} @ search_wire_tile_wire.test\n">() };
			search_wire_tile_wire.test(devRoot);
		}

		{
			auto execLambda{ Timer<"{:10} @ make_wire_tile_node\n">() };
			Search_Wire_Tile_Node::make_wire_tile_node(devRoot);
		}
		Search_Wire_Tile_Node search_wire_tile_node;
		{
			auto execLambda{ Timer<"{:10} @ search_wire_tile_node.test\n">() };
			search_wire_tile_node.test(devRoot);
		}

		{
			auto execLambda{ Timer<"{:10} @ make_node_tile_pip\n">() };
			Search_Node_Tile_Pip::make_node_tile_pip(search_wire_tile_node, devRoot);
		}
		Search_Node_Tile_Pip search_node_tile_pip;
		{
			auto execLambda{ Timer<"{:10} @ search_node_tile_pip.test\n">() };
			search_node_tile_pip.test(search_wire_tile_node, devRoot);
		}

		{
			auto execLambda{ Timer<"{:10} @ make_tile_pip_node\n">() };
			Search_Tile_Pip_Node::make_tile_pip_node(search_node_tile_pip.node_tile_pip);
		}
		Search_Tile_Pip_Node search_tile_pip_node;
		{
			auto execLambda{ Timer<"{:10} @ search_tile_pip_node.test\n">() };
			//search_tile_pip_node.test(search_node_tile_pip.node_tile_pip);
		}

		{
			auto execLambda{ Timer<"{:10} @ make_site_pin_wires\n">() };
			Search_Site_Pin_Wire::make_site_pin_wires(search_wire_tile_wire.wire_tile_wire, devRoot);
		}
		Search_Site_Pin_Wire search_site_pin_wire;
		{
			auto execLambda{ Timer<"{:10} @ search_site_pin_wire.test\n">() };
			// search_site_pin_wire.test(search_wire_tile_wire.wire_tile_wire, devRoot);
		}

		{
			auto execLambda{ Timer<"{:10} @ make_site_pin_nodes\n">() };
			Search_Site_Pin_Node::make_site_pin_nodes(search_wire_tile_node.wire_tile_node, devRoot);
		}
		Search_Site_Pin_Node search_site_pin_node;
		{
			auto execLambda{ Timer<"{:10} @ search_site_pin_node.test\n">() };
			//search_site_pin_node.test(search_wire_tile_node.wire_tile_node, devRoot);
		}

		auto sbg{ TimerVal(String_Building_Group(devRoot.getStrList(), devRoot.getStrList(), devRoot.getTileList())) };
		{
			auto execLambda{ Timer<"{:10} @ Search_Tile_Tile_Wire_Pip::make\n">() };
			Search_Tile_Tile_Wire_Pip::make(devRoot, sbg.dev_tile_strIndex_to_tile, search_wire_tile_node);
		}
		Search_Tile_Tile_Wire_Pip search_tile_tile_wire_pip;
		{
			auto execLambda{ Timer<"{:10} @ search_tile_tile_wire_pip.test\n">() };
			search_tile_tile_wire_pip.test(devRoot, sbg.dev_tile_strIndex_to_tile);
		}
	}

	void tile_based_routing() {
		puts(std::format("devStrs: {}, {} bits", devStrs.size(), ceil(log2(devStrs.size()))).c_str());
		puts(std::format("tiles: {}, {} bits", tiles.size(), ceil(log2(tiles.size()))).c_str());
		puts(std::format("nodes: {}, {} bits", nodes.size(), ceil(log2(nodes.size()))).c_str());
		puts(std::format("wires: {}, {} bits", wires.size(), ceil(log2(wires.size()))).c_str());
		puts(std::format("alt_site_pins: {}, {} bits", rw.alt_site_pins.body.size(), ceil(log2(rw.alt_site_pins.body.size()))).c_str());
		uint32_t max_pip_count{};
		uint32_t max_wire_count{};
		for (auto&& tile_type : tile_types) {
			auto pip_count{ tile_type.getPips().size() };
			auto wire_count{ tile_type.getWires().size() };
			max_pip_count = max_pip_count < pip_count ? pip_count : max_pip_count;
			max_wire_count = max_wire_count < wire_count ? wire_count : max_wire_count;
		}
		puts(std::format("max_pip_count: {}, {} bits", max_pip_count, ceil(log2(max_pip_count))).c_str());
		puts(std::format("max_wire_count: {}, {} bits", max_wire_count, ceil(log2(max_wire_count))).c_str());

		/*
		devStrs: 467843, 19 bits
		tiles: 208370, 18 bits
		nodes: 28226432, 25 bits
		wires: 83282368, 27 bits
		alt_site_pins: 7926429, 23 bits
		max_pip_count: 4083, 12 bits
		max_wire_count: 25285, 15 bits

		tileIdx(18)_tileIdx(18)_ttwire(15)_ttpip(12)

		dense orgin tile, uint32_t of dst tile, pip indx
		*/

#if 1

		auto tile_bounds{ Tile_Info::get_tile_info(tiles) };
		//std::unordered_set<uint32_t> dirty_tiles;
		//dirty_tiles.reserve(tiles.size());

		auto upti{make_ti()};
		std::span<TileInfo, tile_count> ti{ *upti };
		// std::unordered_map<Site_Pin_Tag, uint32_t> site_pin_node;

		auto up_nodes_tiles{ std::make_unique<std::array<Tile_Index, wire_count>>() };
		std::span<Tile_Index, wire_count> s_nodes_tiles{ *up_nodes_tiles };

		auto up_nodes_spans_tiles{ std::make_unique<std::array<std::span<Tile_Index>, node_count>>() };
		std::span<std::span<Tile_Index>, node_count> s_nodes_spans_tiles{ *up_nodes_spans_tiles };

		// std::unordered_map<Wire_Info, uint32_t> wire_info_to_node_idx;

		// wire_info_to_node_idx.reserve(wire_count);


		uint32_t s_nodes_spans_tiles_offset{};

#if 0
		puts("s_nodes_tiles start");
		each(nodes, [&](auto node_idx, auto &node) {
			auto node_wires{ node.getWires() };
			DeviceResources::Device::Wire::Reader node_wire_0{ wires[node_wires[0]] };
			if (wireTypes[node_wire_0.getType()].getCategory() == DeviceResources::Device::WireCategory::GLOBAL) return; //not global clock
			decltype(auto) node_tiles{ s_nodes_spans_tiles[node_idx] };
			node_tiles = s_nodes_tiles.subspan(s_nodes_spans_tiles_offset, node_wires.size());
			s_nodes_spans_tiles_offset += node_wires.size();

			each(node_wires, [&](auto node_wire_idx, auto wire_idx) {
				auto wire{ wires[wire_idx] };
				auto wire_info{ Wire_Info::from_wire(wire) };
				// wire_info_to_node_idx.insert({ wire_info, node_idx });
				auto tile_idx{ sbg.dev_tile_strIndex_to_tile.at(wire.getTile()) };
				node_tiles[node_wire_idx] = Tile_Index::make(tiles[tile_idx]);
			});
		});
		puts("s_nodes_tiles finish");

		puts("pips start");
		for (auto&& tile_info : ti) {
			auto tile_type_wire_strs{ tile_info.tile_type.getWires() };
			auto tile{ tile_info.tile };
			auto tile_type_pips{ tile_info.tile_type.getPips() };

			WireTileNode key{.tileStrIdx{tile.getName()} };

			// auto found{ std::ranges::equal_range(tile_node, key) };
			// auto key_start{ tile_node.data() - found.data() };
			// std::span<WireTileNode> sub_wire_tile_node{ wire_tile_node.subspan(key_start, found.size()) };

			auto tile_range{ std::ranges::equal_range(search_wire_tile_node.wire_tile_node, key, [](WireTileNode a, WireTileNode b) {return a.tileStrIdx < b.tileStrIdx; }) };

			for (auto&& pip : tile_type_pips) {
				auto wire0{ pip.getWire0() };
				auto wire1{ pip.getWire1() };
				auto directional{ pip.getDirectional() };
				auto isConventional{ pip.isConventional() };
				if (!isConventional) continue;

				Wire_Info wi0{ ._wire_strIdx{._strIdx{tile_type_wire_strs[wire0]}}, ._tile_strIdx{._strIdx{tile.getName()}} };
				Wire_Info wi1{ ._wire_strIdx{._strIdx{tile_type_wire_strs[wire1]}}, ._tile_strIdx{._strIdx{tile.getName()}} };

				auto node0_idx{ search_wire_tile_node.wire_tile_to_node(tile_range, wi0.get_tile_strIdx(), wi0.get_wire_strIdx()) };
				auto node1_idx{ search_wire_tile_node.wire_tile_to_node(tile_range, wi1.get_tile_strIdx(), wi1.get_wire_strIdx()) };

				if (node0_idx != UINT32_MAX) {
					decltype(auto) node_tiles{ s_nodes_spans_tiles[node0_idx] };
					for (auto&& tile_idx : node_tiles) {
						tile_info.reachable_tiles.insert(tile_idx);
						tile_info.in_tiles.insert(tile_idx);
						if (!directional) {
							tile_info.out_tiles.insert(tile_idx);
						}
					}
				}

				if (node1_idx != UINT32_MAX) {
					decltype(auto) node_tiles{ s_nodes_spans_tiles[node1_idx] };
					for (auto&& tile_idx : node_tiles) {
						tile_info.reachable_tiles.insert(tile_idx);
						tile_info.out_tiles.insert(tile_idx);
						if (!directional) {
							tile_info.in_tiles.insert(tile_idx);
						}
					}
				}
			}
		}
		puts("pips finish");
#endif

		std::vector<std::shared_ptr<Stub_Router>> stub_routers;

		std::atomic<uint32_t> stub_router_count{};

		puts("placing nets in tiles, start");
		each(physRoot.getPhysNets(), [&](uint64_t net_idx, net_reader &net) {
			auto sources{ net.getSources() };
			auto stubs{ net.getStubs() };
			if (sources.size() != 1) return;
			if (!stubs.size()) return;

			std::set<uint32_t> source_tiles, stub_tiles;
			// puts("sources: ");
			auto source_wires{ search_site_pin_wire.source_site_pins_to_wires(physStrs, search_site_pin_wire.site_pin_wires, sbg.phys_stridx_to_dev_stridx, net.getName(), sources)};
			for (auto wire_idx : source_wires) {
				auto wire{ wires[wire_idx] };
				auto tile{ tiles[sbg.dev_tile_strIndex_to_tile.at(wire.getTile())] };
				auto tile_idx{ Tile_Index::make(tile) };
				auto &tin{ ti[tile_idx._value] };


				source_tiles.insert(tile_idx._value);
				auto node_idx{ search_wire_tile_node.wire_tile_to_node({wire.getTile()}, {wire.getWire()}) };
				tin.unhandled_in_nets.insert(node_idx);
			}

			for (auto&& stub : stubs) {
				auto routeSegment{ stub.getRouteSegment() };
				auto sitePin{ routeSegment.getSitePin() };
				auto wire_idx{ search_site_pin_wire.site_pin_to_wire(
					sbg.phys_stridx_to_dev_stridx.at(sitePin.getSite()),
					sbg.phys_stridx_to_dev_stridx.at(sitePin.getPin())
				) };

				auto wire{ wires[wire_idx] };
				auto tile{ tiles[sbg.dev_tile_strIndex_to_tile.at(wire.getTile())] };
				auto tile_idx{ Tile_Index::make(tile) };
				auto& tin{ ti[tile_idx._value] };


				stub_tiles.insert(tile_idx._value);
				auto node_idx{ search_wire_tile_node.wire_tile_to_node({wire.getTile()}, {wire.getWire()}) };
				tin.unhandled_out_nets.emplace_back(stub_routers.emplace_back(std::make_shared<Stub_Router>(Stub_Router{
					.net_idx{static_cast<uint32_t>(net_idx)},
					.stub{stub},
					.nodes{{node_idx}},
					.source_tiles{std::vector<Tile_Index>{source_tiles.begin(), source_tiles.end()}},
					.tile_path{std::vector<Tile_Index>{tile_idx}},
					.current_distance{HUGE_VAL},
				})).get());
				stub_router_count++;
			}
#if 0
			auto stubs_site_pin_wires{ search_site_pin_wire.site_pins_to_wires(physStrs, search_site_pin_wire.site_pin_wires, sbg.phys_stridx_to_dev_stridx, net.getName(), stubs) };
			for (auto wire_idx : stubs_site_pin_wires) {
				auto wire{ wires[wire_idx] };
				auto tile{ tiles[sbg.dev_tile_strIndex_to_tile.at(wire.getTile())] };
				auto tile_idx{ Tile_Index::make(tile) };
				auto &tin{ ti[tile_idx._value] };


				stub_tiles.insert(tile_idx._value);
				auto node_idx{ search_wire_tile_node.wire_tile_to_node({wire.getTile()}, {wire.getWire()}) };
				decltype(auto) sr{ stub_routers.emplace_back(Stub_Router{
					.net_idx{static_cast<uint32_t>(net_idx)},
				}) };
				tin.unhandled_out_nets.emplace_back(&sr);
			}
#endif
			std::vector<TileInfo*> stub_tile_refs;
			for (auto stub_tile_idx : stub_tiles) {
				stub_tile_refs.emplace_back(&ti[stub_tile_idx]);
			}

			// puts(std::format("source_tiles:{}, stub_tiles:{}, stub_tile_refs: {}", source_tiles.size(), stub_tiles.size(), stub_tile_refs.size()).c_str());

		});

		puts("placing nets in tiles finished\nRouting started");

		// block_phys();
#endif

		const auto start = std::chrono::steady_clock::now();

		{
			std::vector<std::jthread> threads;
			uint64_t group_size{ std::thread::hardware_concurrency() };
			puts(std::format("std::thread::hardware_concurrency: {}", group_size).c_str());

			std::barrier<> bar{ static_cast<ptrdiff_t>(group_size) };
			std::atomic<uint32_t> stubs_to_handle{};
			for (uint64_t offset{ 0 }; offset < group_size; offset++) {
				threads.emplace_back([offset, group_size, &ti, &stub_router_count, &bar, &stubs_to_handle, this]() {
					route_tiles::route_tiles(
						routed_indices,
						routed_index_count,
						unrouted_index_count,
						search_node_tile_pip,
						search_tile_tile_wire_pip,
						offset,
						group_size,
						ti,
						stub_router_count,
						bar,
						stubs_to_handle
					);
				});
			}
			for (auto&& thread : threads) {
				thread.join();
			}
		}

		const auto end = std::chrono::steady_clock::now();

		const std::chrono::duration<double> diff = end - start;

		puts(std::format("finished routing {}s", diff.count()).c_str());
	}

	std::atomic<uint32_t> stubs_further{};
	std::atomic<uint32_t> stubs_deadend{};
	std::atomic<uint32_t> stubs_finished{};


};

