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
	using tile_reader = DeviceResources::Device::Tile::Reader;
	// using branch_builder_span = std::span<branch_builder>;
	using branch_builder_map = std::unordered_map<uint32_t, branch_builder>;
	using u32_span = std::span<uint32_t>;
	using branch_reader_map = std::unordered_map<uint32_t, branch_reader>;
	using tile_reader_map = std::unordered_map<uint32_t, std::vector<tile_reader>>;

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
	std::vector<uint32_t> stored_nodes_nets;
	// ::capnp::MallocMessageBuilder message{ 0x1FFFFFFFu, ::capnp::AllocationStrategy::FIXED_SIZE };
	::capnp::MallocMessageBuilder message{ };
	PhysicalNetlist::PhysNetlist::Builder physBuilder{ message.initRoot<PhysicalNetlist::PhysNetlist>() };

	std::vector<uint32_t> phys_stridx_to_dev_stridx;
	std::unordered_map<uint32_t, uint32_t> dev_strIdx_to_phys_strIdx;
	std::vector<uint32_t> dev_tile_strIndex_to_tile;

	Route_Phys() :
		phys_stridx_to_dev_stridx{ std::vector<uint32_t>( static_cast<size_t>(physStrs.size()), UINT32_MAX ) },
		dev_tile_strIndex_to_tile{ std::vector<uint32_t>(static_cast<size_t>(devStrs.size()), UINT32_MAX) }
	{
		stored_nodes_nets.resize(rw.alt_nodes.size(), UINT32_MAX);

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

	void block_site_pin(uint32_t net_idx, ::PhysicalNetlist::PhysNetlist::PhysSitePin::Reader sitePin) {
		auto ps_source_site{ sitePin.getSite() };
		auto ps_source_pin{ sitePin.getPin() };
		// OutputDebugStringA(std::format("block_site_pin({}, {})\n", strList[ps_source_site].cStr(), strList[ps_source_pin].cStr()).c_str());
		auto ds_source_site{ phys_stridx_to_dev_stridx.at(ps_source_site) };
		auto ds_source_pin{ phys_stridx_to_dev_stridx.at(ps_source_pin) };
		// OutputDebugStringA(std::format("block_site_pin({}, {})\n", dev.strList[ds_source_site].cStr(), dev.strList[ds_source_pin].cStr()).c_str());
		
		auto source_wire_idx{ rw.find_site_pin_wire(ds_source_site, ds_source_pin) };
		if (source_wire_idx != UINT32_MAX) {
			auto source_node_idx{ rw.alt_wire_to_node[source_wire_idx] };
			store_node(net_idx, source_node_idx);
		}
	}

	void block_pip(uint32_t net_idx, ::PhysicalNetlist::PhysNetlist::PhysPIP::Reader pip) {
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

		store_node(net_idx, node0_idx);
		store_node(net_idx, node1_idx);
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

	using RouteIDs = std::unordered_map<uint32_t, std::unordered_set<uint32_t>>;

	RouteIDs build_route_ids(branch_reader_map& pip_stubs) const {
		RouteIDs route_ids;
		{
			std::vector<uint32_t> pips_to_process;
			for (auto&& pip_stub : pip_stubs) pips_to_process.emplace_back(pip_stub.first);

			while (pips_to_process.size()) {
				auto pip_idx{ pips_to_process.back() };
				pips_to_process.pop_back();
				auto prev{ rw.alt_route_storage[pip_idx & 0x7fffffff].previous };
				if (prev != UINT32_MAX) pips_to_process.emplace_back(prev);
				if (route_ids.contains(prev)) {
					route_ids.at(prev).insert(pip_idx);
				}
				else {
					route_ids.insert({ prev, {pip_idx} });
				}
			}
		}
		return route_ids;
	}

	void build_tree_branch(uint32_t net_idx, RouteIDs& route_ids, branch_builder source, branch_reader_map& pip_stubs, uint32_t pip_idx) {
		auto route_id_pips{ route_ids.contains(pip_idx) ? route_ids.at(pip_idx) : std::unordered_set<uint32_t>{} };
		auto source_branches{ source.initBranches(static_cast<uint32_t>(static_cast<size_t>( pip_stubs.contains(pip_idx)) + route_id_pips.size())) };
		uint32_t offset{};
		if (pip_stubs.contains(pip_idx)) {
			source_branches.setWithCaveats(offset++, pip_stubs.extract(pip_idx).mapped());
			fully_routed++;
		}
		for (auto route_id_pip : route_id_pips) {
			store_pip(net_idx, route_id_pip);
			auto branch_n{ source_branches[offset++] };

			auto psi_tile{ get_phys_strIdx_from_dev_strIdx(rw.get_pip_tile0_str(route_id_pip)) };
			auto psi_wire0{ get_phys_strIdx_from_dev_strIdx(rw.get_pip_wire0_str(route_id_pip)) };
			auto psi_wire1{ get_phys_strIdx_from_dev_strIdx(rw.get_pip_wire1_str(route_id_pip)) };

			auto sub_rs{ branch_n.initRouteSegment() };
			auto sub_pip{ sub_rs.initPip() };

			sub_pip.setTile(psi_tile);
			sub_pip.setWire0(psi_wire0);
			sub_pip.setWire1(psi_wire1);
			sub_pip.setForward(route_id_pip >> 31);
			sub_pip.setIsFixed(false);

			build_tree_branch(net_idx, route_ids, branch_n, pip_stubs, route_id_pip);
		}
	}

	void build_tree_base(uint32_t net_idx, RouteIDs& route_ids, branch_builder source, branch_reader_map& pip_stubs, std::unordered_set<uint32_t> base_pips) {
		auto source_branches{ source.initBranches(static_cast<uint32_t>(base_pips.size())) };
		uint32_t offset{};
		for (auto route_id_pip : base_pips) {
			store_pip(net_idx, route_id_pip);
			auto branch_n{ source_branches[offset++] };

			auto psi_tile{ get_phys_strIdx_from_dev_strIdx(rw.get_pip_tile0_str(route_id_pip)) };
			auto psi_wire0{ get_phys_strIdx_from_dev_strIdx(rw.get_pip_wire0_str(route_id_pip)) };
			auto psi_wire1{ get_phys_strIdx_from_dev_strIdx(rw.get_pip_wire1_str(route_id_pip)) };

			auto sub_rs{ branch_n.initRouteSegment() };
			auto sub_pip{ sub_rs.initPip() };

			sub_pip.setTile(psi_tile);
			sub_pip.setWire0(psi_wire0);
			sub_pip.setWire1(psi_wire1);
			sub_pip.setForward(route_id_pip >> 31);
			sub_pip.setIsFixed(false);

			build_tree_branch(net_idx, route_ids, branch_n, pip_stubs, route_id_pip);
		}
	}

	void build_tree(uint32_t net_idx, RouteIDs &route_ids, branch_builder_map& sources, branch_reader_map& pip_stubs, uint32_t source_pip_idx = UINT32_MAX) {
		std::unordered_map<uint32_t, std::unordered_set<uint32_t>> node_pips;//node_idx:pips
		for (auto&& pip_idx : route_ids.at(source_pip_idx)) {
			auto node_in{ rw.get_pip_node_in(pip_idx) };
			auto node_out{ rw.get_pip_node_out(pip_idx) };
			if (node_pips.contains(node_in)) {
				node_pips.at(node_in).insert(pip_idx);
			}
			else {
				node_pips.insert({ node_in, {pip_idx} });
			}
		}
		for (auto&& node_pips_n : node_pips) {
			auto node_in{ node_pips_n.first };
			decltype(sources.extract(node_in)) source{ sources.extract(node_in) };
			build_tree_base(net_idx, route_ids, source.mapped(), pip_stubs, node_pips_n.second);
		}
	}

	void store_route(uint32_t net_idx, branch_builder_map& sources, branch_reader_map &pip_stubs) {
		auto route_ids{ build_route_ids(pip_stubs) };
		puts(std::format("route_ids.size:{}", route_ids.size()).c_str());
		build_tree(net_idx, route_ids, sources, pip_stubs);
		puts("brach_stored");

#if 0
		std::vector<uint32_t> route_ids{};
		route_ids.reserve(rw.alt_route_storage[route_index & 0x7fffffff].past_cost);
		for (uint32_t current_route_index{ route_index }; current_route_index != UINT32_MAX; current_route_index = rw.alt_route_storage[current_route_index & 0x7fffffff].previous) {
			route_ids.emplace_back(current_route_index);
			// OutputDebugStringA(std::format("current_route_index: {}, past_cost: {}\n", current_route_index, ro.storage[current_route_index].get_past_cost()).c_str());
		}
		// OutputDebugStringA(std::format("route_ids.size: {}\n", route_ids.size()).c_str());

		auto current_branches{ branch.initBranches(1) };
//		size_t it_idx_next{};
		for (auto&& it{ route_ids.crbegin() }; it != route_ids.crend(); it++) {
//			it_idx_next++;
			auto current_route_index{ *it };

			store_pip(net_idx, current_route_index);
			
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
#endif
		// fully_routed += pip_stubs.size();
		// if (!(fully_routed % 1000ui32)) display_route_status();
		// OutputDebugStringA(std::format("fully_routed: {}, route_ids.size: {}\n", fully_routed, route_ids.size()).c_str());

	}

	inline bool is_node_stored(uint32_t net_idx, uint32_t node_idx) const {
		auto stored_net{ stored_nodes_nets[node_idx] };
		if (stored_net != UINT32_MAX && stored_net != net_idx) return true;
		return false;
	}

	inline bool are_nodes_stored(uint32_t net_idx, branch_builder_map& nodes) const {
		for (auto&& node : nodes) {
			if (is_node_stored(net_idx, node.first)) return true;
		}
		return false;
	}

	inline void store_node(uint32_t net_idx, uint32_t node_idx) {
		if (stored_nodes_nets[node_idx] != UINT32_MAX && stored_nodes_nets[node_idx] != net_idx) {
			puts(std::format("store net_idx:{} blocking conflict stored_nodes_nets[node_idx:{}]={}", net_idx, node_idx, stored_nodes_nets[node_idx]).c_str());
			abort();
		}

		stored_nodes_nets[node_idx] = net_idx;
	}

	inline bool is_pip_stored(uint32_t net_idx, uint32_t pip_idx) const {
		if (pip_idx == UINT32_MAX) return false;
		auto pip_node_in{ rw.get_pip_node_in(pip_idx) };
		auto pip_node_out{ rw.get_pip_node_out(pip_idx) };
		if (is_node_stored(net_idx, pip_node_in)) return true;
		if (is_node_stored(net_idx, pip_node_out)) return true;
		return false;
	}

	inline void store_pip(uint32_t net_idx, uint32_t pip_idx) {
		auto node_in{ rw.get_pip_node_in(pip_idx) };
		auto node_out{ rw.get_pip_node_out(pip_idx) };

		if (stored_nodes_nets[node_in] != UINT32_MAX && stored_nodes_nets[node_in] != net_idx) {
			puts(std::format("store net_idx:{} blocking conflict f:{} stored_nodes_nets[node_in:{}]={}", net_idx, rw.is_pip_forward(pip_idx), node_in, stored_nodes_nets[node_in]).c_str());
			abort();
		}

		if (stored_nodes_nets[node_out] != UINT32_MAX && stored_nodes_nets[node_out] != net_idx) {
			puts(std::format("store net_idx:{} blocking conflict f:{} stored_nodes_nets[node_out:{}]={}", net_idx, rw.is_pip_forward(pip_idx), node_out, stored_nodes_nets[node_out]).c_str());
			abort();
		}
		stored_nodes_nets[node_in] = net_idx;
		stored_nodes_nets[node_out] = net_idx;
	}

	std::vector<bool> used_nodes;

	std::vector<bool> used_pips;

	inline bool is_node_used(uint32_t node_idx) const {
		return used_nodes[node_idx];
	}

	inline void use_node(uint32_t node_idx) {
		used_nodes[node_idx] = true;
	}

	inline void use_nodes(branch_builder_map& nodes) {
		for (auto&& node : nodes) {
			use_node(node.first);
		}
	}

	inline bool is_pip_used(uint32_t pip_idx) const {
		if (pip_idx == UINT32_MAX) return false;
		return used_pips[pip_idx & 0x7fffffff];
	}

	inline void use_pip(uint32_t pip_idx) {
		if (pip_idx == UINT32_MAX) return;
		used_pips[pip_idx & 0x7fffffff] = true;
	}

	inline void append_pip(uint32_t net_idx, uint32_t pip_idx, uint32_t previous, uint16_t past_cost, uint16_t future_cost) {
		if (is_pip_stored(net_idx, pip_idx)) return;
		if (is_pip_used(pip_idx)) return;
		rw.append(pip_idx, previous, past_cost, future_cost);
		use_pip(pip_idx);
	}

	inline void add_node_pip(uint32_t net_idx, tile_reader_map& stub_tiles, uint32_t node_idx, uint32_t pip_idx, uint32_t parent_pip_idx) {
		if (is_pip_stored(net_idx, pip_idx)) return;
		if (is_pip_used(pip_idx & 0x7fffffff)) return;

		bool pip_idx_forward{ static_cast<bool>(pip_idx >> 31u) };

		auto node_in{ rw.get_pip_node_in(pip_idx) };
		auto node_out{ rw.get_pip_node_out(pip_idx) };

		if (is_node_used(node_out)) return;
		use_node(node_out);

		uint32_t best_dist{ UINT32_MAX };

		for (auto&& node_tile : stub_tiles) {
			auto stub_node_tiles{ node_tile.second };

			for (auto&& stub_node_tile : stub_node_tiles) {
				for (auto&& node_wire_out : rw.alt_nodes[node_out]) {
					auto node_wire_out_tile{ tiles[dev_tile_strIndex_to_tile.at(rw.get_wire_tile_str(node_wire_out))] };

					auto node_wire_out_colDiff{ static_cast<int32_t>(stub_node_tile.getCol()) - static_cast<int32_t>(node_wire_out_tile.getCol()) };
					auto node_wire_out_rowDiff{ static_cast<int32_t>(stub_node_tile.getRow()) - static_cast<int32_t>(node_wire_out_tile.getRow()) };
					auto dist{ abs(node_wire_out_colDiff) + abs(node_wire_out_rowDiff) };
					best_dist = dist < best_dist ? dist : best_dist;
				}
			}
		}

		uint16_t past_cost{ (parent_pip_idx == UINT32_MAX) ? static_cast<uint16_t>(0) : static_cast<uint16_t>(rw.alt_route_storage[parent_pip_idx & 0x7fffffffu].past_cost + 1ull) };
		append_pip(net_idx, pip_idx, parent_pip_idx, past_cost, best_dist);

	}

	void add_node_pips(uint32_t net_idx, tile_reader_map &stub_tiles, uint32_t node_idx, uint32_t parent_pip_idx = UINT32_MAX) {
		if (is_pip_stored(net_idx, parent_pip_idx)) return;

		for (auto&& source_pip : rw.alt_node_to_pips[node_idx]) {
			add_node_pip(net_idx, stub_tiles, node_idx, source_pip, parent_pip_idx);
		}
	}

	void add_nodes_pips(uint32_t net_idx, tile_reader_map &stub_tiles, branch_builder_map& source_nodes, uint32_t parent_pip_idx = UINT32_MAX) {
		for (auto&& node_idx : source_nodes) {
			add_node_pips(net_idx, stub_tiles, node_idx.first, parent_pip_idx);
		}
	}

	bool route_stub(uint32_t net_idx, branch_builder_map &sources, branch_reader_map &stubs, tile_reader_map &stub_tiles) {
		rw.clear_routes();
		used_nodes.clear(); used_nodes.resize(rw.alt_nodes.size(), false);
		used_pips.clear();  used_pips.resize(rw.alt_pips.size(), false);

		if (are_nodes_stored(net_idx, sources)) {
			puts("stored_nodes_nets[source_node_idx] != net_idx");
			abort();
		}

		use_nodes(sources);

		add_nodes_pips(net_idx, stub_tiles, sources);

		branch_reader_map pip_stubs;

		const uint32_t chunk_size{ static_cast<uint32_t>(rw.alt_nodes.size()) };
		for (uint32_t attempts{}; attempts <= chunk_size; attempts++) {
			if (attempts > 0 && !(attempts % chunk_size)) puts(std::format("fully_routed: {}, failed_route: {}, attempts: {}, q: {}", fully_routed, failed_route, attempts, rw.alt_route_options.size()).c_str());
			if (!rw.alt_route_options.size()) {
				puts(std::format("EMPTY fully_routed: {}, failed_route: {}, attempts: {}, q: {}", fully_routed, failed_route, attempts, rw.alt_route_options.size()).c_str());
				break;
			}

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

			uint32_t parent_pip_idx{ rw.alt_route_options.top() };
			rw.alt_route_options.pop();

			auto node_out_idx{ rw.get_pip_node_out(parent_pip_idx) };
			if (stubs.contains(node_out_idx)) {
				if (is_pip_stored(net_idx, parent_pip_idx)) {
					puts("is_pip_stored(net_idx, parent_pip_idx) && node_out_idx == stub_node_idx");
					abort();
				}
				auto stub_node{ stubs.extract(node_out_idx) };
				stub_tiles.erase(node_out_idx);
				stub_node.key() = parent_pip_idx;
				pip_stubs.insert(std::move(stub_node));
				if (!stubs.size()) {
					store_route(net_idx, sources, pip_stubs);
					return true;
				}
			}

			add_node_pips(net_idx, stub_tiles, node_out_idx, parent_pip_idx);
		}

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

	void branches_site_pin(uint32_t nameIdx, branch_list_builder branches, std::vector<branch_builder> &ret) {
		for (auto&& branch : branches) {
			auto name{ physStrs[nameIdx] };
			auto branch_rs{ branch.getRouteSegment() };
			auto branch_branches{ branch.getBranches() };
			auto branch_rs_which{ branch_rs.which() };
			// OutputDebugStringA(std::format("{} branches({}) which {}\n", name.cStr(), branch_branches.size(), static_cast<uint16_t>(branch_rs_which)).c_str());
			switch (branch_rs_which) {
			case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::BEL_PIN: {
				branches_site_pin(nameIdx, branch_branches, ret);
				break;
			}
			case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::SITE_PIN: {
				if (!branch_branches.size()) ret.emplace_back(branch);
				branches_site_pin(nameIdx, branch_branches, ret);
				break;
			}
			case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::PIP: {
				puts("PIP");
				abort();
				break;
			}
			case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::SITE_P_I_P: {
				branches_site_pin(nameIdx, branch_branches, ret);
				break;

			}
			default: {
				puts("unreachable");
				abort();
			}
			}
		}
	}

	bool assign_stubs(uint32_t net_idx, uint32_t nameIdx, branch_list_builder sources, branch_list_reader stubs) {
		std::vector<branch_builder> source_site_pins;
		branches_site_pin(nameIdx, sources, source_site_pins);
		branch_builder_map source_nodes;

		for (auto&& site_pin : source_site_pins) {
			auto ps_source_site{ site_pin.getRouteSegment().getSitePin().getSite() };
			auto ps_source_pin{ site_pin.getRouteSegment().getSitePin().getPin() };
			auto ds_source_site{ phys_stridx_to_dev_stridx.at(ps_source_site) };
			auto ds_source_pin{ phys_stridx_to_dev_stridx.at(ps_source_pin) };
			auto source_wire_idx{ rw.find_site_pin_wire(ds_source_site, ds_source_pin) };
			auto source_node_idx{ rw.alt_wire_to_node[source_wire_idx] };
			source_nodes.insert({ source_node_idx, site_pin });
		}

		branch_reader_map stubs_map;
		tile_reader_map stub_tiles;

		for (auto&& stub : stubs) {
			auto ps_stub_site{ stub.getRouteSegment().getSitePin().getSite() };
			auto ps_stub_pin{ stub.getRouteSegment().getSitePin().getPin() };
			auto ds_stub_site{ phys_stridx_to_dev_stridx.at(ps_stub_site) };
			auto ds_stub_pin{ phys_stridx_to_dev_stridx.at(ps_stub_pin) };
			auto stub_wire_idx{ rw.find_site_pin_wire(ds_stub_site, ds_stub_pin) };
			auto stub_node_idx{ rw.alt_wire_to_node[stub_wire_idx] };
			auto stub_tile{ tiles[dev_tile_strIndex_to_tile.at(rw.get_wire_tile_str(stub_wire_idx))] };
			auto stub_node_wires{ rw.alt_nodes[stub_node_idx] };

			std::unordered_set<uint32_t> stub_node_tile_set;
			std::vector<tile_reader> stub_node_tiles{};
			stub_node_tiles.reserve(stub_node_wires.size());

			for (auto&& stub_wire_idx : stub_node_wires) {
				auto stub_node_wire_tile_strIdx{ rw.get_wire_tile_str(stub_wire_idx) };
				auto stub_node_wire_tile_idx{ dev_tile_strIndex_to_tile.at(stub_node_wire_tile_strIdx) };
				if (!stub_node_tile_set.contains(stub_node_wire_tile_idx)) {
					stub_node_tile_set.emplace(stub_node_wire_tile_idx);
					stub_node_tiles.emplace_back(tiles[stub_node_wire_tile_idx]);
				}
			}
			stubs_map.insert({ stub_node_idx , stub });
			stub_tiles.insert({ stub_node_idx, std::move(stub_node_tiles)});
		}

		return route_stub(net_idx, source_nodes, stubs_map, stub_tiles);
	}

	void route() {
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
			if (!(n % 100)) {
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

			if (r_sources.size() > 1u && r_stubs.size() > 1u) {
				puts(std::format("{} sources:{}, stubs:{}\n", physStrs[phyNetReader.getName()].cStr(), r_sources.size(), r_stubs.size()).c_str());
			} else if (r_sources.size() == 1u && r_stubs.size() > 0u && fully_routed < 1000) {
				if (!assign_stubs(n, phyNetReader.getName(), b_sources, r_stubs)) {
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

		phys.write(message);

		puts("route finish");

	}
};

