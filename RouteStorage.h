#pragma once

#include "RenumberedWires.h"
#include "interchange_types.h"

#include "NodeStorage.h"
#include <unordered_set>
#include <unordered_map>
#include "PIP_Index.h"

class Route_Info {
public:
    alignas(sizeof(uint64_t))
    PIP_Index previous;
    uint16_t past_cost;
    uint16_t future_cost;

    inline uint32_t get_total_cost() const noexcept { return static_cast<uint32_t>(past_cost) + static_cast<uint32_t>(future_cost); }
};

static_assert(sizeof(::Route_Info) == 8);

class Route_Info_Comparison {
public:
    std::span<::Route_Info> alt_route_storage;
    Route_Info_Comparison(std::span<::Route_Info> alt_route_storage) : alt_route_storage{ alt_route_storage } {}
    bool operator() (PIP_Index left, PIP_Index right) {
        auto left_pip{ alt_route_storage[left.get_id()]};
        auto right_pip{ alt_route_storage[right.get_id()] };
        if (left_pip.get_total_cost() > right_pip.get_total_cost()) return true;
        if (left_pip.get_total_cost() == right_pip.get_total_cost()) {
            if (left_pip.past_cost < right_pip.past_cost) return true;
        }
        return false;
    }
};
using RoutePriorityQueue = std::priority_queue<PIP_Index, std::vector<PIP_Index>, ::Route_Info_Comparison>;

#include "String_Building_Group.h"

class RouteStorage {
public:
    ::MemoryMappedFile route_storage_mmf;
    std::span<::Route_Info> alt_route_storage;

    ::Route_Info_Comparison ric;

    RoutePriorityQueue alt_route_options;
    NodeStorage& ns; // should be const at some point

    upNodeBitset up_used_nodes;
    upPipBitset up_used_pips;

    rNodeBitset r_used_nodes;
    rPipBitset r_used_pips;

    crNodeBitset cr_used_nodes;
    crPipBitset cr_used_pips;
	const RenumberedWires &rw;
	String_Building_Group &sbg;

	std::span<uint32_t> unrouted_indices;
	std::atomic<uint32_t> &unrouted_index_count;

	std::span<uint32_t> stubs_indices;
	std::atomic<uint32_t> &stubs_index_count;


	RouteStorage(
		size_t alt_pips_size,
		decltype(ns) ns,
		decltype(rw) rw,
		decltype(sbg) sbg,
		decltype(unrouted_indices) unrouted_indices,
		decltype(unrouted_index_count) unrouted_index_count,
		decltype(stubs_indices) stubs_indices,
		decltype(stubs_index_count) stubs_index_count
	) noexcept :
        route_storage_mmf{ "route_storage2.bin", alt_pips_size * sizeof(::Route_Info) },
        alt_route_storage{ route_storage_mmf.get_span<::Route_Info>() },
        ric{ alt_route_storage },
        alt_route_options{ ric },
        ns{ ns },
        up_used_nodes{ std::make_unique<_NodeBitset>() },
        up_used_pips{ std::make_unique<_PipBitset>() },
        r_used_nodes{ *up_used_nodes.get() },
        r_used_pips{ *up_used_pips.get() },
        cr_used_nodes{ r_used_nodes },
        cr_used_pips{ r_used_pips },
		rw{ rw }, sbg{ sbg },
		unrouted_indices{ unrouted_indices }, unrouted_index_count{ unrouted_index_count },
		stubs_indices{ stubs_indices }, stubs_index_count{ stubs_index_count }
	{
		unrouted_index_count = 0;
		stubs_index_count = 0;
    }

    inline void append(PIP_Index pip_idx, PIP_Index previous, uint16_t past_cost, uint16_t future_cost) noexcept {
        alt_route_storage[pip_idx.get_id()].previous = previous;
        alt_route_storage[pip_idx.get_id()].past_cost = past_cost;
        alt_route_storage[pip_idx.get_id()].future_cost = future_cost;

        alt_route_options.emplace(pip_idx); // not constexpr
    }

    inline constexpr void clear_routes() noexcept {
        alt_route_options = RoutePriorityQueue{ ric };
    }

    inline constexpr bool is_node_used(uint32_t node_idx) const {
        return cr_used_nodes[node_idx];
    }

	inline constexpr void use_node(uint32_t node_idx) noexcept {
		r_used_nodes[node_idx] = true;
	}

	inline constexpr static void use_node(rNodeBitset r_used_nodes, uint32_t node_idx) noexcept {
        r_used_nodes[node_idx] = true;
    }

	inline static void use_nodes(rNodeBitset r_used_nodes, branch_builder_map& nodes /* not constexpr */) noexcept {
		for (auto&& node : nodes) {
			use_node(r_used_nodes, node.first);
		}
	}

	inline void use_nodes(branch_builder_map& nodes /* not constexpr */) noexcept {
		for (auto&& node : nodes) {
			use_node(r_used_nodes, node.first);
		}
	}

	inline constexpr bool is_pip_used(PIP_Index pip_idx) const noexcept {
        if (pip_idx.is_root()) return false;
        return cr_used_pips[pip_idx.get_id()];
    }

	inline constexpr static void use_pip(rNodeBitset r_used_pips, PIP_Index pip_idx) noexcept {
		if (pip_idx.is_root()) return;
		r_used_pips[pip_idx.get_id()] = true;
	}

	inline constexpr void use_pip(PIP_Index pip_idx) noexcept {
		if (pip_idx.is_root()) return;
		r_used_pips[pip_idx.get_id()] = true;
	}

	inline constexpr void append_pip(uint32_t net_idx, PIP_Index pip_idx, PIP_Index previous, uint16_t past_cost, uint16_t future_cost) noexcept {
        if (ns.is_pip_stored(net_idx, pip_idx)) return;
        if (is_pip_used(pip_idx)) return;
        append(pip_idx, previous, past_cost, future_cost);
        use_pip(pip_idx);

		if (!unrouted_indices.empty() && !previous.is_root() && !pip_idx.is_root() && unrouted_index_count + 2 < unrouted_indices.size()) {
			auto prev_tile_idx{ sbg.dev_tile_strIndex_to_tile.at(rw.get_pip_tile0_str(previous)._strIdx) };
			auto cur_tile_idx{ sbg.dev_tile_strIndex_to_tile.at(rw.get_pip_tile0_str(pip_idx)._strIdx) };
			unrouted_indices[unrouted_index_count++] = prev_tile_idx;
			unrouted_indices[unrouted_index_count++] = cur_tile_idx;
		}
    }


	using RouteIDs = std::unordered_map<PIP_Index, std::unordered_set<PIP_Index>>;

	RouteIDs build_route_ids(branch_reader_pip_map& pip_stubs) const {
		RouteIDs route_ids;
		{
			std::vector<PIP_Index> pips_to_process;
			for (auto&& pip_stub : pip_stubs) pips_to_process.emplace_back(pip_stub.first);

			while (pips_to_process.size()) {
				auto pip_idx{ pips_to_process.back() };
				pips_to_process.pop_back();
				auto prev{ alt_route_storage[pip_idx.get_id()].previous };
				if (!prev.is_root()) pips_to_process.emplace_back(prev);
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

	void build_tree_branch(uint32_t net_idx, RouteIDs& route_ids, branch_builder source, branch_reader_pip_map& pip_stubs, PIP_Index pip_idx) {
		auto route_id_pips{ route_ids.contains(pip_idx) ? route_ids.at(pip_idx) : std::unordered_set<PIP_Index>{} };
		auto source_branches{ source.initBranches(static_cast<uint32_t>(static_cast<size_t>(pip_stubs.contains(pip_idx)) + route_id_pips.size())) };
		uint32_t offset{};
		if (pip_stubs.contains(pip_idx)) {
			source_branches.setWithCaveats(offset++, pip_stubs.extract(pip_idx).mapped());
		}
		for (auto route_id_pip : route_id_pips) {
			ns.store_pip(net_idx, route_id_pip);
			auto branch_n{ source_branches[offset++] };

			auto psi_tile{ sbg.get_phys_strIdx_from_dev_strIdx(rw.get_pip_tile0_str(route_id_pip)) };
			auto psi_wire0{ sbg.get_phys_strIdx_from_dev_strIdx(rw.get_pip_wire0_str(route_id_pip)) };
			auto psi_wire1{ sbg.get_phys_strIdx_from_dev_strIdx(rw.get_pip_wire1_str(route_id_pip)) };

			auto sub_rs{ branch_n.initRouteSegment() };
			auto sub_pip{ sub_rs.initPip() };

			sub_pip.setTile(psi_tile._strIdx);
			sub_pip.setWire0(psi_wire0._strIdx);
			sub_pip.setWire1(psi_wire1._strIdx);
			sub_pip.setForward(route_id_pip.is_pip_forward());
			sub_pip.setIsFixed(false);

			build_tree_branch(net_idx, route_ids, branch_n, pip_stubs, route_id_pip);
		}
	}

	void build_tree_base(uint32_t net_idx, RouteIDs& route_ids, branch_builder source, branch_reader_pip_map& pip_stubs, std::unordered_set<PIP_Index> base_pips) {
		auto source_branches{ source.initBranches(static_cast<uint32_t>(base_pips.size())) };
		uint32_t offset{};
		for (auto route_id_pip : base_pips) {
			ns.store_pip(net_idx, route_id_pip);
			auto branch_n{ source_branches[offset++] };

			auto psi_tile{ sbg.get_phys_strIdx_from_dev_strIdx(rw.get_pip_tile0_str(route_id_pip)) };
			auto psi_wire0{ sbg.get_phys_strIdx_from_dev_strIdx(rw.get_pip_wire0_str(route_id_pip)) };
			auto psi_wire1{ sbg.get_phys_strIdx_from_dev_strIdx(rw.get_pip_wire1_str(route_id_pip)) };

			auto sub_rs{ branch_n.initRouteSegment() };
			auto sub_pip{ sub_rs.initPip() };

			sub_pip.setTile(psi_tile._strIdx);
			sub_pip.setWire0(psi_wire0._strIdx);
			sub_pip.setWire1(psi_wire1._strIdx);
			sub_pip.setForward(route_id_pip.is_pip_forward());
			sub_pip.setIsFixed(false);

			build_tree_branch(net_idx, route_ids, branch_n, pip_stubs, route_id_pip);
		}
	}

	void build_tree(uint32_t net_idx, RouteIDs& route_ids, branch_builder_map& sources, branch_reader_pip_map& pip_stubs, PIP_Index source_pip_idx = PIP_Index_Root) {
		std::unordered_map<uint32_t, std::unordered_set<PIP_Index>> node_pips;//node_idx:pips
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

	void store_route(uint32_t net_idx, branch_builder_map& sources, branch_reader_pip_map& pip_stubs) {
		auto route_ids{ build_route_ids(pip_stubs) };
		build_tree(net_idx, route_ids, sources, pip_stubs);
	}


	inline auto get_tile_from_wire(uint32_t wire_idx) const noexcept {
		return sbg.tiles[sbg.dev_tile_strIndex_to_tile.at(rw.get_wire_tile_str(wire_idx)._strIdx)];
	}


	inline void add_node_pip(uint32_t net_idx, tile_reader_map& stub_tiles, uint32_t node_idx, PIP_Index pip_idx, PIP_Index parent_pip_idx) {
		if (ns.is_pip_stored(net_idx, pip_idx)) return;
		if (is_pip_used(pip_idx)) return;

		bool pip_idx_forward{ pip_idx.is_pip_forward() };

		auto node_in{ rw.get_pip_node_in(pip_idx) };
		auto node_out{ rw.get_pip_node_out(pip_idx) };

		if (is_node_used(node_out)) return;
		use_node(node_out);

		uint32_t best_dist{ UINT32_MAX };

		for (auto&& node_tile : stub_tiles) {
			auto stub_node_tiles{ node_tile.second };

			for (auto&& stub_node_tile : stub_node_tiles) {
				for (auto&& node_wire_out : rw.alt_nodes[node_out]) {
					auto node_wire_out_tile{ get_tile_from_wire(node_wire_out) };

					auto node_wire_out_colDiff{ static_cast<int32_t>(stub_node_tile.getCol()) - static_cast<int32_t>(node_wire_out_tile.getCol()) };
					auto node_wire_out_rowDiff{ static_cast<int32_t>(stub_node_tile.getRow()) - static_cast<int32_t>(node_wire_out_tile.getRow()) };
					uint32_t dist{ (static_cast<uint32_t>(abs(node_wire_out_colDiff)) + static_cast<uint32_t>(abs(node_wire_out_rowDiff))) * 10u };
					best_dist = (dist < best_dist) ? dist : best_dist;
				}
			}
		}

		uint16_t past_cost{ (parent_pip_idx.is_root()) ? static_cast<uint16_t>(0) : static_cast<uint16_t>(get_past_cost(parent_pip_idx) + 1ull)};
		append_pip(net_idx, pip_idx, parent_pip_idx, past_cost, best_dist);

	}

	inline constexpr uint16_t get_past_cost(PIP_Index pip_idx) const noexcept {
		return alt_route_storage[pip_idx.get_id()].past_cost;
	}

	void add_node_pips(uint32_t net_idx, tile_reader_map& stub_tiles, uint32_t node_idx, PIP_Index parent_pip_idx = PIP_Index_Root) {
		if (ns.is_pip_stored(net_idx, parent_pip_idx)) return;

		for (auto&& source_pip : rw.alt_node_to_pips[node_idx]) {
			add_node_pip(net_idx, stub_tiles, node_idx, source_pip, parent_pip_idx);
		}
	}

	void add_nodes_pips(uint32_t net_idx, tile_reader_map& stub_tiles, branch_builder_map& source_nodes, PIP_Index parent_pip_idx = PIP_Index_Root) {
		for (auto&& node_idx : source_nodes) {
			add_node_pips(net_idx, stub_tiles, node_idx.first, parent_pip_idx);
		}
	}

	inline PIP_Index pop_top() noexcept {
		auto ret{ alt_route_options.top() };
		alt_route_options.pop();
		return ret;
	}

	bool route_stub(uint32_t net_idx, branch_builder_map& sources, branch_reader_node_map& stubs, tile_reader_map& stub_tiles) {
		if (ns.are_nodes_stored(net_idx, sources)) {
			puts("stored_nodes_nets[source_node_idx] != net_idx");
			abort();
		}

		use_nodes(sources);

		add_nodes_pips(net_idx, stub_tiles, sources);

		branch_reader_pip_map pip_stubs;

		const uint32_t chunk_size{ 10000 };
		const uint32_t max_attempts{ 1000000 };
		// const uint32_t max_attempts{ static_cast<uint32_t>(rw.alt_nodes.size()) };

		for (uint32_t attempts{}; attempts <= max_attempts; attempts++) {
#if 0
			total_attempts++;
			if (attempts > 0 && !(attempts % chunk_size)) puts(std::format("skip_route: {}, fully_routed: {}, failed_route: {}, attempts: {}, q: {}", skip_route, fully_routed, failed_route, attempts, rw.alt_route_options.size()).c_str());
#endif

			if (!alt_route_options.size()) {
				puts(std::format("EMPTY attempts: {}", attempts).c_str());
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

			auto parent_pip_idx{ pop_top() };

			auto node_out_idx{ rw.get_pip_node_out(parent_pip_idx) };
			if (stubs.contains(node_out_idx)) {
				if (ns.is_pip_stored(net_idx, parent_pip_idx)) {
					puts("is_pip_stored(net_idx, parent_pip_idx) && node_out_idx == stub_node_idx");
					abort();
				}
				auto stub_node{ stubs.extract(node_out_idx) };
				stub_tiles.erase(node_out_idx);
				pip_stubs.insert({ parent_pip_idx, stub_node.mapped() });
				if (!stubs.size()) {
					store_route(net_idx, sources, pip_stubs);
					// fully_routed.increment();
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

	static void branches_site_pin(uint32_t nameIdx, branch_list_builder branches, std::vector<branch_builder>& ret) noexcept {
		for (auto&& branch : branches) {
			// auto name{ physStrs[nameIdx] };
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
		std::vector<uint32_t> source_tiles;

		for (auto&& site_pin : source_site_pins) {
			auto ps_source_site{ site_pin.getRouteSegment().getSitePin().getSite() };
			auto ps_source_pin{ site_pin.getRouteSegment().getSitePin().getPin() };
			auto ds_source_site{ sbg.phys_stridx_to_dev_stridx.at(ps_source_site) };
			auto ds_source_pin{ sbg.phys_stridx_to_dev_stridx.at(ps_source_pin) };
			auto source_wire_idx{ rw.find_site_pin_wire(ds_source_site, ds_source_pin) };
			auto source_node_idx{ rw.alt_wire_to_node[source_wire_idx] };
			source_nodes.insert({ source_node_idx, site_pin });
			source_tiles.emplace_back(sbg.dev_tile_strIndex_to_tile.at(rw.get_wire_tile_str(source_wire_idx)._strIdx));
		}

		branch_reader_node_map stubs_map;
		tile_reader_map stub_tiles;

		for (auto&& stub : stubs) {
			auto ps_stub_site{ stub.getRouteSegment().getSitePin().getSite() };
			auto ps_stub_pin{ stub.getRouteSegment().getSitePin().getPin() };
			auto ds_stub_site{ sbg.phys_stridx_to_dev_stridx.at(ps_stub_site) };
			auto ds_stub_pin{ sbg.phys_stridx_to_dev_stridx.at(ps_stub_pin) };
			auto stub_wire_idx{ rw.find_site_pin_wire(ds_stub_site, ds_stub_pin) };
			auto stub_node_idx{ rw.alt_wire_to_node[stub_wire_idx] };
			auto stub_tile{ get_tile_from_wire(stub_wire_idx) };
			auto stub_node_wires{ rw.alt_nodes[stub_node_idx] };

			std::unordered_set<uint32_t> stub_node_tile_set;
			std::vector<tile_reader> stub_node_tiles{};
			stub_node_tiles.reserve(stub_node_wires.size());

			for (auto&& stub_wire_idx : stub_node_wires) {
				auto stub_node_wire_tile_strIdx{ rw.get_wire_tile_str(stub_wire_idx) };
				auto stub_node_wire_tile_idx{ sbg.dev_tile_strIndex_to_tile.at(stub_node_wire_tile_strIdx._strIdx) };
				if (!stub_node_tile_set.contains(stub_node_wire_tile_idx)) {
					stub_node_tile_set.emplace(stub_node_wire_tile_idx);
					stub_node_tiles.emplace_back(sbg.tiles[stub_node_wire_tile_idx]);
					stubs_indices[stubs_index_count++] = source_tiles[0];
					stubs_indices[stubs_index_count++] = stub_node_wire_tile_idx;
				}
			}
			stubs_map.insert({ stub_node_idx , stub });
			stub_tiles.insert({ stub_node_idx, std::move(stub_node_tiles) });
		}

		return route_stub(net_idx, source_nodes, stubs_map, stub_tiles);
	}

};

