#pragma once

#include "DeviceResources.capnp.h"
#include "LogicalNetlist.capnp.h"
#include "PhysicalNetlist.capnp.h"

class Route_Phys {
	using branch = ::PhysicalNetlist::PhysNetlist::RouteBranch;
	using branch_reader = ::PhysicalNetlist::PhysNetlist::RouteBranch::Reader;
	using branch_builder = ::PhysicalNetlist::PhysNetlist::RouteBranch::Builder;
	using branch_list = ::capnp::List<branch, ::capnp::Kind::STRUCT>;
	using branch_list_builder = branch_list::Builder;
	using branch_list_reader = branch_list::Reader;

public:
	DevGZ dev{ "_deps/device-file-src/xcvu3p.device" };
	PhysGZ phys{ "_deps/benchmark-files-src/boom_soc_unrouted.phys" };
	DeviceResources::Device::Reader devRoot{ dev.root };
	PhysicalNetlist::PhysNetlist::Reader physRoot{ phys.root };
	::capnp::List< ::capnp::Text, ::capnp::Kind::BLOB>::Reader devStrs{ devRoot.getStrList() };
	::capnp::List< ::capnp::Text, ::capnp::Kind::BLOB>::Reader physStrs{ physRoot.getStrList() };
	::capnp::List< ::DeviceResources::Device::Tile, ::capnp::Kind::STRUCT>::Reader tiles{devRoot.getTileList()};
	std::vector<uint32_t> extra_dev_strIdx;
	uint32_t fully_routed{};
	uint32_t failed_route{};

	RenumberedWires rw;
	std::vector<bool> stored_nodes;
	::capnp::MallocMessageBuilder message{/* words */ };
	PhysicalNetlist::PhysNetlist::Builder physBuilder{ message.initRoot<PhysicalNetlist::PhysNetlist>() };

	std::vector<uint32_t> phys_stridx_to_dev_stridx;
	std::unordered_map<uint32_t, uint32_t> dev_strIdx_to_phys_strIdx;
	std::vector<uint32_t> dev_tile_strIndex_to_tile;

	Route_Phys() :
		phys_stridx_to_dev_stridx{ std::vector<uint32_t>( static_cast<size_t>(physStrs.size()), UINT32_MAX ) },
		dev_tile_strIndex_to_tile{ std::vector<uint32_t>(static_cast<size_t>(devStrs.size()), UINT32_MAX) }
	{
		stored_nodes.resize(rw.alt_nodes.size());

		puts(std::format("Route_Phys() start").c_str());

		for (uint32_t tile_idx{}; tile_idx < tiles.size(); tile_idx++) {
			auto tile{ tiles[tile_idx] };
			dev_tile_strIndex_to_tile[tile.getName()] = tile_idx;
		}
		std::unordered_map<std::string_view, std::uint32_t> dev_strmap;
		dev_strmap.reserve(devStrs.size());
		for (uint32_t dev_strIdx{}; dev_strIdx < devStrs.size(); dev_strIdx++) {
			auto dev_str{ devStrs[dev_strIdx] };
			dev_strmap.insert({ dev_str.cStr(), dev_strIdx });
		}
		for (uint32_t phys_strIdx{}; phys_strIdx < physStrs.size(); phys_strIdx++) {
			std::string_view phys_str{ physStrs[phys_strIdx].cStr() };
			if (dev_strmap.contains(phys_str)) {
				auto dev_strIdx{ dev_strmap.at(phys_str) };
				phys_stridx_to_dev_stridx[phys_strIdx] = dev_strIdx;
				dev_strIdx_to_phys_strIdx.insert({ dev_strIdx, phys_strIdx });
			}
		}

		puts(std::format("Route_Phys() finish").c_str());
	}

	void block_site_pin(::PhysicalNetlist::PhysNetlist::PhysSitePin::Reader sitePin) {
		auto ps_source_site{ sitePin.getSite() };
		auto ps_source_pin{ sitePin.getPin() };
		// OutputDebugStringA(std::format("block_site_pin({}, {})\n", strList[ps_source_site].cStr(), strList[ps_source_pin].cStr()).c_str());
		auto ds_source_site{ phys_stridx_to_dev_stridx.at(ps_source_site) };
		auto ds_source_pin{ phys_stridx_to_dev_stridx.at(ps_source_pin) };
		// OutputDebugStringA(std::format("block_site_pin({}, {})\n", dev.strList[ds_source_site].cStr(), dev.strList[ds_source_pin].cStr()).c_str());
		
		auto source_wire_idx{ rw.find_site_pin_wire(ds_source_site, ds_source_pin) };
		if (source_wire_idx != UINT32_MAX) {
			auto source_node_idx{ rw.alt_wire_to_node[source_wire_idx] };
			stored_nodes[source_node_idx] = true;
		}
	}

	void block_pip(::PhysicalNetlist::PhysNetlist::PhysPIP::Reader pip) {
		auto ps_tile{ pip.getTile() };
		auto ps_wire0{ pip.getWire0() };
		auto ps_wire1{ pip.getWire1() };

		auto ds_tile{ phys_stridx_to_dev_stridx.at(ps_tile) };
		auto ds_wire0{ phys_stridx_to_dev_stridx.at(ps_wire0) };
		auto ds_wire1{ phys_stridx_to_dev_stridx.at(ps_wire1) };

		auto wire0_idx{ rw.find_wire(ds_tile, ds_wire0) };
		auto wire1_idx{ rw.find_wire(ds_tile, ds_wire1) };


		auto node0_idx{ rw.alt_wire_to_node[wire0_idx] };
		auto node1_idx{ rw.alt_wire_to_node[wire1_idx] };

		stored_nodes[node0_idx] = true;
		stored_nodes[node1_idx] = true;
	}

	void block_source_resource(PhysicalNetlist::PhysNetlist::RouteBranch::Reader branch) {
		auto rs{ branch.getRouteSegment() };
		switch (rs.which()) {
		case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::Which::BEL_PIN: {
			auto belPin{ rs.getBelPin() };
			// block_bel_pin(belPin);
			break;
		}
		case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::Which::SITE_PIN: {
			auto sitePin{ rs.getSitePin() };
			block_site_pin(sitePin);
			break;
		}
		case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::Which::PIP: {
			auto pip{ rs.getPip() };
			block_pip(pip);
			break;
		}
		case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::Which::SITE_P_I_P: {
			auto sitePip{ rs.getSitePIP() };
			// block_site_pip(sitePip);
			break;
		}
		default:
			abort();
		}
		for (auto&& sub_branch : branch.getBranches()) {
			block_source_resource(sub_branch);
		}
	}

	void block_resources(PhysicalNetlist::PhysNetlist::PhysNet::Reader physNet) {
		for (auto&& src_branch : physNet.getSources()) {
			block_source_resource(src_branch);
		}
		for (auto&& stub_branch : physNet.getStubs()) {
			block_source_resource(stub_branch);
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

	uint32_t get_phys_strIdx_from_dev_strIdx(uint32_t dev_strIdx) {
		if (dev_strIdx_to_phys_strIdx.contains(dev_strIdx)) {
			return dev_strIdx_to_phys_strIdx.at(dev_strIdx);
		}
		uint32_t ret{ static_cast<uint32_t>(physStrs.size() + extra_dev_strIdx.size()) };
		extra_dev_strIdx.emplace_back(dev_strIdx);
		phys_stridx_to_dev_stridx.emplace_back(dev_strIdx);
		dev_strIdx_to_phys_strIdx.insert({ dev_strIdx, ret });
		return ret;
	}

	void store_route(branch_builder branch, branch_reader stub, uint32_t route_index) {
		std::vector<uint32_t> route_ids{};
		route_ids.reserve(rw.alt_route_storage[route_index & 0x7fffffff].past_cost);
		for (uint32_t current_route_index{ route_index }; current_route_index != UINT32_MAX; current_route_index = rw.alt_route_storage[current_route_index & 0x7fffffff].previous) {
			route_ids.emplace_back(current_route_index);
			// OutputDebugStringA(std::format("current_route_index: {}, past_cost: {}\n", current_route_index, ro.storage[current_route_index].get_past_cost()).c_str());
		}
		// OutputDebugStringA(std::format("route_ids.size: {}\n", route_ids.size()).c_str());

		auto current_branches{ branch.initBranches(1) };
		for (auto&& it{ route_ids.crbegin() }; it != route_ids.crend(); it++) {
			auto current_route_index{ *it };
			// OutputDebugStringA(std::format("current_route_index: {}, past_cost: {}\n", current_route_index, ro.storage[current_route_index].get_past_cost()).c_str());

			//OutputDebugStringA("Wire match\n");
			
			stored_nodes[rw.get_pip_node0(current_route_index)] = true;
			stored_nodes[rw.get_pip_node1(current_route_index)] = true;

			
			auto psi_tile{ get_phys_strIdx_from_dev_strIdx(rw.get_pip_tile0_str(current_route_index)) };
			auto psi_wire0{ get_phys_strIdx_from_dev_strIdx(rw.get_pip_wire0_str(current_route_index)) };
			auto psi_wire1{ get_phys_strIdx_from_dev_strIdx(rw.get_pip_wire1_str(current_route_index)) };
			// return;
			auto sub_rs{ current_branches[0].initRouteSegment() };
			auto sub_pip{ sub_rs.initPip() };

			sub_pip.setTile(psi_tile);
			sub_pip.setWire0(psi_wire0);
			sub_pip.setWire1(psi_wire1);
			sub_pip.setForward(current_route_index >> 31);
			sub_pip.setIsFixed(false);

			current_branches = current_branches[0].initBranches(1);

		}

		current_branches.setWithCaveats(0, stub);
		fully_routed++;
		// if (!(fully_routed % 1000ui32)) display_route_status();
		// OutputDebugStringA(std::format("fully_routed: {}, route_ids.size: {}\n", fully_routed, route_ids.size()).c_str());

	}

	bool route_stub(branch_builder branch, branch_reader stub, uint32_t source_wire_idx, uint32_t stub_wire_idx, std::vector<uint32_t> stub_node_tiles) {
		rw.clear_routes();

		std::vector<bool> used_nodes;
		used_nodes.resize(rw.alt_nodes.size(), false);

		auto source_node_idx{ rw.alt_wire_to_node[source_wire_idx] };
		auto stub_node_idx{ rw.alt_wire_to_node[stub_wire_idx] };

		used_nodes[source_node_idx] = true;

		// puts(std::format("s").c_str());

		auto source_node_pips{ rw.alt_node_to_pips[source_node_idx] };
		for (auto pip_idx : source_node_pips) {
			rw.append(pip_idx, UINT32_MAX, 0, 0);
		}
		const uint32_t chunk_size{ 1000 };
		for (uint32_t attempts{}; attempts <= chunk_size; attempts++) {
			if (attempts > 0 && !(attempts % chunk_size)) puts(std::format("fully_routed: {}, failed_route: {}, attempts: {}, q: {}", fully_routed, failed_route, attempts, rw.alt_route_options.size()).c_str());
			if (!rw.alt_route_options.size()) {
				// puts("Empty");
				break;
			}
			uint32_t top{ rw.alt_route_options.top() };
			bool top_forward{ static_cast<bool>(top >> 31u) };
			if (!top_forward) continue;
			rw.alt_route_options.pop();
			auto top_info{ rw.alt_route_storage[top & 0x7fffffffu] };
			auto wire0_idx{ rw.get_pip_wire0(top) };
			auto wire1_idx{ rw.get_pip_wire1(top) };

			auto node_in_idx{ top_forward?rw.get_pip_node0(top): rw.get_pip_node1(top) };
			auto node_out_idx{ top_forward?rw.get_pip_node1(top): rw.get_pip_node0(top) };

			if (node_out_idx == stub_node_idx) {
				store_route(branch, stub, top);
				return true;
			}
			if (used_nodes[node_out_idx]) continue;

			used_nodes[node_out_idx] = true;

#if 0
			std::print("fully_routed: {}, failed_route: {}, attempts: {}, count:{}, topID:{}, wire0:{}, wire1:{}, p:{}, f:{}, t:{}\n",
				fully_routed,
				failed_route,
				attempts,
				rw.alt_route_options.size(),
				top,
				wire0_idx,
				wire1_idx,
				static_cast<uint64_t>(top_info.past_cost),
				static_cast<uint64_t>(top_info.future_cost),
				top_info.get_total_cost()
			);
#endif

			auto current_pips{ rw.alt_node_to_pips[node_out_idx] };

			for (auto&& source_pip : current_pips) {
				bool source_pip_forward{ static_cast<bool>(source_pip >> 31u) };
				if (!source_pip_forward) continue;
				if (rw.alt_route_storage[source_pip & 0x7fffffff].is_used()) continue;
				auto wire_in{ source_pip_forward?rw.get_pip_wire0(source_pip): rw.get_pip_wire1(source_pip) };
				auto wire_out{ source_pip_forward?rw.get_pip_wire1(source_pip): rw.get_pip_wire0(source_pip) };
				auto node_out{ source_pip_forward?rw.get_pip_node1(source_pip): rw.get_pip_node0(source_pip) };

				if (node_out == stub_node_idx) {
					rw.append(source_pip, top, top_info.past_cost + 1, 0);
					store_route(branch, stub, source_pip);
					// puts("store_route");
					// puts(std::format("fully_routed: {}, failed_route: {}", fully_routed, failed_route).c_str());
					return true;
				}

				if (stored_nodes[node_out]) continue;
				used_nodes[node_out] = true;

				for (auto&& node_wire_out : rw.alt_nodes[node_out]) {
					auto node_wire_out_tile_idx{ dev_tile_strIndex_to_tile.at(rw.get_wire_tile_str(node_wire_out)) };
					auto node_wire_out_tile{ tiles[node_wire_out_tile_idx] };
					auto node_wire_out_tileCol{ node_wire_out_tile.getCol() };
					auto node_wire_out_tileRow{ node_wire_out_tile.getRow() };

					for (auto&& stub_node_tile_idx : stub_node_tiles) {
						auto stub_node_tile{ tiles[stub_node_tile_idx] };
						auto node_wire_out_colDiff{ static_cast<int32_t>(stub_node_tile.getCol()) - static_cast<int32_t>(node_wire_out_tile.getCol()) };
						auto node_wire_out_rowDiff{ static_cast<int32_t>(stub_node_tile.getRow()) - static_cast<int32_t>(node_wire_out_tile.getRow()) };
						auto dist{ abs(node_wire_out_colDiff) + abs(node_wire_out_rowDiff) };
						rw.append(source_pip, top, top_info.past_cost + 1, dist);
					}
				}
			}
		}

		failed_route++;
#if 0
		if (ro.q5.empty()) {
			OutputDebugStringA(std::format("fully_routed: {}, failed_route: {}, success_rate: {} empty\n",
				fully_routed,
				failed_route,
				static_cast<double_t>(fully_routed * 100ui32) / static_cast<double_t>(fully_routed + failed_route)).c_str());
		}
		else {
			OutputDebugStringA(std::format("fully_routed: {}, failed_route: {}, success_rate: {}, count:{}, past_cost:{}, future_cost:{} fail\n",
				fully_routed,
				failed_route,
				static_cast<double_t>(fully_routed * 100ui32) / static_cast<double_t>(fully_routed + failed_route),
				ro.q5.size(),
				ro.storage[ro.q5.top()].get_past_cost(),
				ro.storage[ro.q5.top()].get_future_cost()).c_str());
		}
#endif

		// OutputDebugStringA("\n\n");
		return false;
	}

	bool assign_stub(uint32_t nameIdx, branch_builder branch, branch_reader stub) {
		auto site_pin_optional{ branch_site_pin(nameIdx, branch) };
		if (!site_pin_optional.has_value()) {
			puts("source lacking site_pin");
			abort();
		}
		auto site_pin{ site_pin_optional.value() };
		if (site_pin.getBranches().size()) {
			puts("site_pin.getBranches().size()");
			abort();
		}
		if (!stub.getRouteSegment().isSitePin()) {
			puts("not stub site pin");
			abort();
		}
		auto ps_source_site{ site_pin.getRouteSegment().getSitePin().getSite() };
		auto ps_source_pin{ site_pin.getRouteSegment().getSitePin().getPin() };
		auto ps_stub_site{ stub.getRouteSegment().getSitePin().getSite() };
		auto ps_stub_pin{ stub.getRouteSegment().getSitePin().getPin() };
#if 0
		std::print("source {}:{}, stub {}:{}\n",
			physStrs[ps_source_site].cStr(),
			physStrs[ps_source_pin].cStr(),
			physStrs[ps_stub_site].cStr(),
			physStrs[ps_stub_pin].cStr());
#endif

		auto ds_source_site{ phys_stridx_to_dev_stridx.at(ps_source_site) };
		auto ds_source_pin{ phys_stridx_to_dev_stridx.at(ps_source_pin) };
		auto ds_stub_site{ phys_stridx_to_dev_stridx.at(ps_stub_site) };
		auto ds_stub_pin{ phys_stridx_to_dev_stridx.at(ps_stub_pin) };

#if 0
		std::print("source {}:{}, stub {}:{}\n",
			devStrs[ds_source_site].cStr(),
			devStrs[ds_source_pin].cStr(),
			devStrs[ds_stub_site].cStr(),
			devStrs[ds_stub_pin].cStr()
		);
#endif

		auto source_wire_idx{ rw.find_site_pin_wire(ds_source_site, ds_source_pin) };
		auto stub_wire_idx{ rw.find_site_pin_wire(ds_stub_site, ds_stub_pin) };
		auto stub_tile{ tiles[dev_tile_strIndex_to_tile.at(rw.get_wire_tile_str(stub_wire_idx))] };

		auto source_node_idx{ rw.alt_wire_to_node[source_wire_idx] };
		auto stub_node_idx{ rw.alt_wire_to_node[stub_wire_idx] };
		auto stub_node_wires{ rw.alt_nodes[stub_node_idx] };

		std::unordered_set<uint32_t> stub_node_tile_set;
		std::vector<uint32_t> stub_node_tiles{};
		stub_node_tiles.reserve(stub_node_wires.size());
#if 1

		for (auto&& stub_wire_idx : stub_node_wires) {
			auto stub_node_wire_tile_strIdx{ rw.get_wire_tile_str(stub_wire_idx) };
			auto stub_node_wire_tile{ dev_tile_strIndex_to_tile.at(stub_node_wire_tile_strIdx) };
			if (!stub_node_tile_set.contains(stub_node_wire_tile)) {
				stub_node_tile_set.emplace(stub_node_wire_tile);
				stub_node_tiles.emplace_back(stub_node_wire_tile);
			}
		}

		return route_stub(site_pin, stub, source_wire_idx, stub_wire_idx, stub_node_tiles);
#endif
		return false;
	}

	void check_conflicts(PhysicalNetlist::PhysNetlist::Builder physBuilder) {

	}

	void route() {
		puts(std::format("route start").c_str());

		physBuilder.setPart(physRoot.getPart());
		physBuilder.setPlacements(physRoot.getPlacements());
		auto readerPhysNets{ physRoot.getPhysNets() };
		uint32_t readerPhysNets_size{ readerPhysNets.size() };
		physBuilder.initPhysNets(readerPhysNets_size);
		auto listPhysNets{ physBuilder.getPhysNets() };
		for (auto&& phyNetReader : readerPhysNets) {
			block_resources(phyNetReader);
		}
		for (uint32_t n{}; n != readerPhysNets_size; n++) {
			if (!(n % 1000)) {
				puts(std::format("n: {} of {}, fully_routed: {}, failed_route: {}", n, readerPhysNets_size, fully_routed, failed_route).c_str());
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

			if (r_sources.size() == 1u && r_stubs.size() == 1u && fully_routed < 1000) {
				// assign_stubs(b_sources, r_stubs);
				if (!assign_stub(phyNetReader.getName(), b_sources[0], r_stubs[0])) {
					phyNetBuilder.setStubs(r_stubs);
					failed_route++;
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
		auto strListBuilder{ physBuilder.initStrList(static_cast<uint32_t>(physStrs.size() + extra_dev_strIdx.size())) };

		for (uint32_t strIdx{}; strIdx < physStrs.size(); strIdx++) {
			strListBuilder.set(strIdx, physStrs[strIdx]);
		}

		for (uint32_t extraIdx{}; extraIdx < extra_dev_strIdx.size(); extraIdx++) {
			auto dev_strIdx{ extra_dev_strIdx[extraIdx] };
			strListBuilder.set(physStrs.size() + extraIdx, devStrs[dev_strIdx]);
		}
		physBuilder.setSiteInsts(physRoot.getSiteInsts());
		physBuilder.setProperties(physRoot.getProperties());
		physBuilder.setNullNet(physRoot.getNullNet());

		check_conflicts(physBuilder);
		// debug_net(phyBuilder, "system/clint/time_00[5]");
		phys.write(message);

		puts("route finish");

	}
};

