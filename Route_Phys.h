#pragma once

#include "interchange_types.h"
#include "InterchangeGZ.h"
#include "RenumberedWires.h"
#include "NodeStorage.h"
#include "String_Building_Group.h"
#include "RouteStorage.h"
#include <thread>

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

class Route_Phys {

public:

	Counter fully_routed, skip_route, failed_route, total_attempts;
	DevGZ dev{ "_deps/device-file-src/xcvu3p.device" };
	PhysGZ phys{ "_deps/benchmark-files-src/boom_med_pb_unrouted.phys" };
	// PhysGZ phys{ "_deps/benchmark-files-src/boom_soc_unrouted.phys" };
	DeviceResources::Device::Reader devRoot{ dev.root };
	PhysicalNetlist::PhysNetlist::Reader physRoot{ phys.root };
	::capnp::List< ::capnp::Text, ::capnp::Kind::BLOB>::Reader devStrs{ devRoot.getStrList() };
	::capnp::List< ::DeviceResources::Device::Tile, ::capnp::Kind::STRUCT>::Reader tiles{ devRoot.getTileList() };
	::capnp::List< ::capnp::Text, ::capnp::Kind::BLOB>::Reader physStrs{ physRoot.getStrList() };

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
};

