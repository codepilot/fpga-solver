#pragma once

#include "interchange_types.h"
#include "InterchangeGZ.h"
#include "RenumberedWires.h"
#include "NodeStorage.h"
#include "String_Building_Group.h"
#include "RouteStorage.h"
#include <thread>
#include "PIP_Index.h"

class Counter {
public:
	uint64_t value{};
	constexpr void increment() noexcept { value++; }
};

struct Tile_Info {
	int64_t minCol;
	int64_t minRow;
	int64_t maxCol;
	int64_t maxRow;
	int64_t numCol;
	int64_t numRow;

	inline constexpr static Tile_Info make() noexcept {
		return {
			.minCol{ INT64_MAX },
			.minRow{ INT64_MAX },
			.maxCol{ INT64_MIN },
			.maxRow{ INT64_MIN },
			.numCol{},
			.numRow{},
		};
	}

	inline constexpr size_t size() const noexcept {
		return numCol * numRow;
	}

	inline constexpr void set_tile(std::span<uint32_t> sp_tile_drawing, int64_t col, int64_t row, uint32_t val) const noexcept {
		sp_tile_drawing[col + row * numCol] = val;
	}

	inline static Tile_Info get_tile_info(::capnp::List< ::DeviceResources::Device::Tile, ::capnp::Kind::STRUCT>::Reader tiles) noexcept {
		auto ret{ Tile_Info::make() };
		for (auto&& tile : tiles) {
			const auto tileName{ tile.getName() };
			const auto col{ tile.getCol() };
			const auto row{ tile.getRow() };
			ret.minCol = (col < ret.minCol) ? col : ret.minCol;
			ret.minRow = (row < ret.minRow) ? row : ret.minRow;
			ret.maxCol = (col > ret.maxCol) ? col : ret.maxCol;
			ret.maxRow = (row > ret.maxRow) ? row : ret.maxRow;
		}
		ret.numCol = ret.maxCol - ret.minCol + 1ull;
		ret.numRow = ret.maxRow - ret.minRow + 1ull;
		return ret;
	}

};

#include "wire_tile_node.h"
#include "site_pin_nodes.h"
#include "node_tile_pip.h"
#include "tile_pip_node.h"

class Route_Phys {

public:

	inline constexpr static size_t tile_count{ 208370ull };
	inline constexpr static size_t wire_count{ 83282368ull };
	inline constexpr static size_t node_count{ 28226432ull };

	Counter fully_routed, skip_route, failed_route, total_attempts;
	DevGZ dev{ "_deps/device-file-src/xcvu3p.device" };
	PhysGZ phys{ "_deps/benchmark-files-src/boom_med_pb_unrouted.phys" };
	// PhysGZ phys{ "_deps/benchmark-files-src/boom_soc_unrouted.phys" };
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

		for (auto &&tile: tiles) {
			unrouted_locations.emplace_back(std::array<uint16_t, 2>{tile.getCol(), tile.getRow()});
		}

		puts(std::format("Route_Phys() finish").c_str());
	}

	void start_routing(std::span<uint32_t> routed_indices_mapping, std::span<uint32_t> unrouted_indices_mapping, std::span<uint32_t> stubs_indices_mapping) {
		routed_indices = routed_indices_mapping;
		unrouted_indices = unrouted_indices_mapping;
		stubs_indices = stubs_indices_mapping;

		jt = std::jthread{ [this](std::stop_token stoken) { route(stoken); } };
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

		phys.write(message);

		puts("route finish");

	}

	void block_phys() {
		puts("block_phys() start");
		each(physRoot.getPhysNets(), [&](uint32_t net_idx, net_reader net) {
			block_resources(net_idx, net);
		});
		puts("block_phys() finish");
	}

	class TileInfo {
	public:
		std::string_view name;
		Tile_Index tile_idx;
		tile_reader tile;
		tile_type_reader tile_type;
		std::set<Tile_Index> in_tiles, out_tiles, reachable_tiles;
	};
	//19 bits str site, 19 bit pin, 25 bit node/branch, 1 bit leaf



	std::unique_ptr<std::array<TileInfo, tile_count>> make_ti() {
		auto upti{ std::make_unique<std::array<TileInfo, tile_count>>() };
		std::span<TileInfo, tile_count> ti{ *upti };

		puts("ti start");
		each(tiles, [&](auto tile_counter, tile_reader tile) {
			auto tile_idx{ Tile_Index::make(tile) };
			decltype(auto) tile_info{ ti[tile_idx._value] };
			tile_info = { .name{devStrs[tile.getName()].cStr()}, .tile_idx{tile_idx}, .tile{tile}, .tile_type{tile_types[tile.getType()]} };
			});
		puts("ti finish");

		return upti;
	}

	void tile_based_routing() {
		puts(std::format("devStrs: {}, {} bits", devStrs.size(), ceil(log2(devStrs.size()))).c_str());
		puts(std::format("tiles: {}, {} bits", tiles.size(), ceil(log2(tiles.size()))).c_str());
		puts(std::format("nodes: {}, {} bits", nodes.size(), ceil(log2(nodes.size()))).c_str());
		puts(std::format("wires: {}, {} bits", wires.size(), ceil(log2(wires.size()))).c_str());
		puts(std::format("alt_site_pins: {}, {} bits", rw.alt_site_pins.body.size(), ceil(log2(rw.alt_site_pins.body.size()))).c_str());
		uint32_t max_pip_count{};
		for (auto&& tile_type : tile_types) {
			auto pip_count{ tile_type.getPips().size() };
			max_pip_count = max_pip_count < pip_count ? pip_count : max_pip_count;
		}
		puts(std::format("max_pip_count: {}, {} bits", max_pip_count, ceil(log2(max_pip_count))).c_str());

		/*
		devStrs: 467843, 19 bits
		tiles: 208370, 18 bits
		nodes: 28226432, 25 bits
		wires: 83282368, 27 bits
		alt_site_pins: 7926429, 23 bits
		max_pip_count: 4083, 12 bits
		*/

		// auto wire_tile_node{ make_wire_tile_node() };
		// save_wire_tile_node();

		// Search_Wire_Tile_Node::save_wire_tile_node(devRoot);
		Search_Wire_Tile_Node search_wire_tile_node;

		// Search_Node_Tile_Pip::save_node_tile_pip(search_wire_tile_node, devRoot);
		Search_Node_Tile_Pip search_node_tile_pip;

		// Search_Tile_Pip_Node::save_tile_pip_node(search_node_tile_pip.node_tile_pip);
		Search_Tile_Pip_Node search_tile_pip_node;

		// Search_Site_Pin_Node::save_site_pin_nodes(search_wire_tile_node.wire_tile_node, devRoot);
		Search_Site_Pin_Node search_site_pin_node;

#if 0

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


		puts("s_nodes_tiles start");
		each(nodes, [&](auto node_idx, auto node) {
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

			auto tile_range{ std::ranges::equal_range(wire_tile_node, key, [](WireTileNode a, WireTileNode b) {return a.tileStrIdx < b.tileStrIdx; }) };

			for (auto&& pip : tile_type_pips) {
				auto wire0{ pip.getWire0() };
				auto wire1{ pip.getWire1() };
				auto directional{ pip.getDirectional() };
				auto isConventional{ pip.isConventional() };
				if (!isConventional) continue;

				Wire_Info wi0{ ._wire_strIdx{._strIdx{tile_type_wire_strs[wire0]}}, ._tile_strIdx{._strIdx{tile.getName()}} };
				Wire_Info wi1{ ._wire_strIdx{._strIdx{tile_type_wire_strs[wire1]}}, ._tile_strIdx{._strIdx{tile.getName()}} };

				auto node0_idx{ wire_tile_to_node(tile_range, wi0.get_tile_strIdx(), wi0.get_wire_strIdx()) };
				auto node1_idx{ wire_tile_to_node(tile_range, wi1.get_tile_strIdx(), wi1.get_wire_strIdx()) };

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

		each(physRoot.getPhysNets(), [&](uint32_t net_idx, net_reader net) {
			auto sources{ net.getSources() };
			auto stubs{ net.getStubs() };
			if (sources.size() != 1) return;
			if (!stubs.size()) return;

			auto source_nodes{ site_pins_to_nodes(site_pin_nodes, net_idx, sources) };
			auto stubs_site_pins{ site_pins_to_nodes(site_pin_nodes, net_idx, stubs) };
		});


		// block_phys();
#endif

	}
};

