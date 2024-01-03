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
	std::vector<bool> stored_nodes{};

	RenumberedWires rw;
	::capnp::MallocMessageBuilder message{/* words */ };
	PhysicalNetlist::PhysNetlist::Builder physBuilder{ message.initRoot<PhysicalNetlist::PhysNetlist>() };

	std::vector<uint32_t> phys_stridx_to_dev_stridx;
	std::vector<uint32_t> dev_tile_strIndex_to_tile;

	Route_Phys() :
		phys_stridx_to_dev_stridx{ std::vector<uint32_t>( static_cast<size_t>(physStrs.size()), UINT32_MAX ) },
		dev_tile_strIndex_to_tile{ std::vector<uint32_t>(static_cast<size_t>(devStrs.size()), UINT32_MAX) }
	{
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
			}
		}
		puts(std::format("Route_Phys() finish").c_str());
	}

	void block_resources(PhysicalNetlist::PhysNetlist::PhysNet::Reader physNet) {

	}

	std::optional<branch_builder> branch_site_pin(uint32_t nameIdx, branch_builder branch) {
		auto name{ physStrs[nameIdx] };
		auto branch_rs{ branch.getRouteSegment() };
		auto branch_branches{ branch.getBranches() };
		auto branch_rs_which{ branch_rs.which() };
		// OutputDebugStringA(std::format("{} branches({}) which {}\n", name.cStr(), branch_branches.size(), static_cast<uint16_t>(branch_rs_which)).c_str());
		switch (branch_rs_which) {
		case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::BEL_PIN: {
			// OutputDebugStringA("BEL_PIN\n");
			if (branch_branches.size()) {
				for (auto&& subbranch : branch_branches) {
					auto ret{ branch_site_pin(nameIdx, subbranch) };
					if (ret.has_value()) return ret;
				}
				return std::nullopt;
			}
			else {
				// OutputDebugStringA("BEL_PIN Empty\n");
				return std::nullopt;
			}
			break;
		}
		case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::SITE_PIN: {
			// OutputDebugStringA("SITE_PIN\n");
			if (branch_branches.size()) {
				for (auto&& subbranch : branch_branches) {
					auto ret{ branch_site_pin(nameIdx, subbranch) };
					if (ret.has_value()) return ret;
				}
				return std::nullopt;
			}
			else {
				// OutputDebugStringA("BEL_PIN Empty\n");
				return branch;
			}
		}
		case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::PIP: {
			puts("PIP");
			abort();
			break;
		}
		case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::SITE_P_I_P: {
			// OutputDebugStringA("SITE_P_I_P\n");
			if (branch_branches.size()) {
				for (auto&& subbranch : branch_branches) {
					auto ret{ branch_site_pin(nameIdx, subbranch) };
					if (ret.has_value()) return ret;
				}
				return std::nullopt;
			}
			else {
				// OutputDebugStringA("SITE_P_I_P Empty\n");
				return std::nullopt;
			}

		}
		default: {
			puts("unreachable");
			abort();
		}
		}
		return std::nullopt;
	}

	bool route_stub(branch_builder branch, branch_reader stub, uint32_t source_wire_idx, uint32_t stub_wire_idx, std::vector<uint32_t> stub_node_tiles) {
		rw.clear_routes();

		auto stub_node_idx{ rw.alt_wire_to_node[stub_wire_idx] };
		auto stub_node_pips{ rw.alt_node_to_pips[stub_node_idx] };
		for (auto pip_idx : stub_node_pips) {
			rw.append(pip_idx, UINT32_MAX, 0, 0xffff);
		}
		const uint32_t chunk_size{ 1000 };
		for (uint32_t attempts{}; attempts <= chunk_size; attempts++) {
			if (attempts > 0 && !(attempts % chunk_size)) puts(std::format("attempts: {}, q: {}", attempts, rw.alt_route_options.size()).c_str());
			if (!rw.alt_route_options.size()) {
				// puts("Empty");
				break;
			}
			uint32_t top{ rw.alt_route_options.top() };
			rw.alt_route_options.pop();
			auto top_info{ rw.alt_route_storage[top] };
			auto wire0_idx{ rw.get_pip_wire0(top) };
			auto wire1_idx{ rw.get_pip_wire1(top) };

			auto node0_idx{ rw.get_pip_node0(top) };
			auto node1_idx{ rw.get_pip_node1(top) };

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

			auto current_wire0{ wire1_idx };
			auto current_node0{ node1_idx };
			auto current_pips{ rw.alt_node_to_pips[current_node0] };


			for (auto&& source_pip : current_pips) {
				if (rw.alt_route_storage[source_pip].is_used()) continue;
				auto wire_in{ rw.get_pip_wire0(source_pip) };
				auto wire_out{ rw.get_pip_wire1(source_pip) };
				auto node_out{ rw.get_pip_node1(source_pip) };

				if (node_out == stub_node_idx) {
					rw.append(source_pip, top, top_info.past_cost + 1, 0);
					// store_route(branch, stub, ro, source_pip);
					// puts("store_route");
					puts(std::format("fully_routed: {}, failed_route: {}", fully_routed, failed_route).c_str());
					return true;
				}
#if 0 // start


				if (stored_nodes[node_out]) continue;

				for (auto&& node_wire_out : dev.nodes[node_out].getWires()) {
					auto node_wire_out_tile_idx{ dev.tile_strIndex_to_tile.at(dev.wires[node_wire_out].getTile()) };
					auto node_wire_out_tile{ dev.tiles[node_wire_out_tile_idx] };
					auto node_wire_out_tileCol{ node_wire_out_tile.getCol() };
					auto node_wire_out_tileRow{ node_wire_out_tile.getRow() };
#if 0
					if (stub_wire_idx == node_wire_out) {
						auto rs_index{ ro.append(wire_in, wire_out, top, top_info.get_past_cost() + 1, 0) };
						store_route(branch, stub, ro, rs_index);

						return true;
					}
#endif
					for (auto&& stub_node_tile_idx : stub_node_tiles) {
						auto stub_node_tile{ dev.tiles[stub_node_tile_idx] };
						auto node_wire_out_colDiff{ static_cast<int32_t>(stub_node_tile.getCol()) - static_cast<int32_t>(node_wire_out_tile.getCol()) };
						auto node_wire_out_rowDiff{ static_cast<int32_t>(stub_node_tile.getRow()) - static_cast<int32_t>(node_wire_out_tile.getRow()) };
						auto dist{ abs(node_wire_out_colDiff) + abs(node_wire_out_rowDiff) };
						auto rs_index{ ro.append(wire_in, wire_out, top, top_info.get_past_cost() + 1, dist) };
						// ro.append(wire_in, wire_out, 0, 0, dist);
						// OutputDebugStringA(std::format("node pip {} => {} => {}, dist: {}\n", it->first, it->second, node_wire_out, dist).c_str());
					}
				}
				// OutputDebugStringA("\n");
#endif
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
			auto phyNetBuilder{ listPhysNets[n] };
			phyNetBuilder.setName(phyNetReader.getName());

			auto r_sources{ phyNetReader.getSources() };
			auto r_stubs{ phyNetReader.getStubs() };

			phyNetBuilder.setSources(r_sources);
			auto b_sources{ phyNetBuilder.getSources() };

			if (r_sources.size() == 1ui32 && r_stubs.size() == 1ui32) {
				// assign_stubs(b_sources, r_stubs);
				if (!assign_stub(phyNetReader.getName(), b_sources[0], r_stubs[0])) {
					phyNetBuilder.setStubs(r_stubs);
					failed_route++;
				}
				else {
					fully_routed++;
				}
				//phyNetBuilder.initStubs(0);
			}
			else {
				phyNetBuilder.setStubs(r_stubs);
			}

			phyNetBuilder.setType(phyNetReader.getType());
			phyNetBuilder.setStubNodes(phyNetReader.getStubNodes());
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

