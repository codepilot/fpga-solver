#pragma once

namespace route_tiles {
	constexpr static always_inline void get_best_initial_tile(
		const Search_Node_Tile_Pip& search_node_tile_pip,
		const Search_Tile_Tile_Wire_Pip& search_tile_tile_wire_pip,
		std::span<const TileInfo, tile_count> cti,
		TileInfo& tin,
		Stub_Router* ustub,
		Tile_Index& bestTI,
		double_t& bestDistance
	) {

		Tile_Index previous{ ._value {INT32_MAX} };

		for (auto&& ntp : search_node_tile_pip.node_to_tile_pip(ustub->nodes.at(0))) {
			auto ti_dest{ std::bit_cast<Tile_Index>(static_cast<uint32_t>(ntp.tile_idx)) };
			if (ti_dest == tin.tile_idx) continue;
			if (ti_dest == previous) continue;
			previous = ti_dest;

			if (ti_dest == ustub->source_tiles[0]) {
				bestDistance = 0;
				bestTI = ti_dest;
				break;
			}

			decltype(auto) dst_ti{ cti[ti_dest._value] };

			auto dest_tile_pips{ search_tile_tile_wire_pip.search_tile_tile_pip[ti_dest._value] };
			if (!dest_tile_pips.size()) {
				// puts(std::format("no pips {} ", dst_ti.name).c_str());
				continue;
			}

			auto dest_tile_type{ cti[ti_dest._value].tile_type };
			if (dest_tile_type.getSiteTypes().size()) {
				// puts(std::format("contains sites {} ", dst_ti.name).c_str());
				continue;
			}

			auto dist{ ustub->source_tiles[0].distance(ti_dest) };
			if (dist < bestDistance) {
				bestDistance = dist;
				bestTI = ti_dest;
			}
		}
	}

	constexpr static always_inline void get_best_next_tile(
		const Search_Tile_Tile_Wire_Pip& search_tile_tile_wire_pip,
		std::span<const TileInfo, tile_count> cti,
		TileInfo& tin,
		Stub_Router* ustub,
		std::span<TilePip> tile_pips,
		Tile_Index& bestTI,
		double_t& bestDistance
	)
	{
		Tile_Index previous{ ._value {INT32_MAX} };
		for (auto tile_pip : tile_pips) {
			auto ti_dest{ std::bit_cast<Tile_Index>(static_cast<uint32_t>(tile_pip.tile_destination)) };
			if (ti_dest == tin.tile_idx) continue;
			if (ti_dest == previous) continue;
			previous = ti_dest;

			if (ti_dest == ustub->source_tiles[0]) {
				bestDistance = 0;
				bestTI = ti_dest;
				break;
			}

			decltype(auto) dst_ti{ cti[ti_dest._value] };

			auto dest_tile_pips{ search_tile_tile_wire_pip.search_tile_tile_pip[ti_dest._value] };
			if (!dest_tile_pips.size()) {
				// puts(std::format("no pips {} ", dst_ti.name).c_str());
				continue;
			}

			auto dest_tile_type{ cti[ti_dest._value].tile_type };
			if (dest_tile_type.getSiteTypes().size()) {
				// puts(std::format("contains sites {} ", dst_ti.name).c_str());
				continue;
			}

			auto dist{ ustub->source_tiles[0].distance(ti_dest) };
			if (dist < bestDistance) {
				bestDistance = dist;
				bestTI = ti_dest;
			}
			// puts(std::format("stub: {} dist: {} dest:{} pip:{}", ustub->net_idx, dist, static_cast<uint32_t>(tile_pip.tile_destination), static_cast<uint32_t>(tile_pip.pip_offset)).c_str());
		}
	}

	constexpr static always_inline void route_tile_stub(
		std::span<uint32_t> routed_indices,
		std::atomic<uint32_t>& routed_index_count,
		std::atomic<uint32_t>& unrouted_index_count,
		const Search_Node_Tile_Pip& search_node_tile_pip,
		const Search_Tile_Tile_Wire_Pip& search_tile_tile_wire_pip,
		std::span<const TileInfo, tile_count> cti,
		std::span<TileInfo, tile_count>& ti,
		TileInfo& tin,
		std::atomic<uint32_t>& stub_router_count,
		std::span<TilePip> tile_pips,
		Stub_Router* ustub
	)
	{
		Tile_Index bestTI{ ._value {INT32_MAX} };
		double_t bestDistance{ HUGE_VAL };
		if (ustub->tile_path.size() == 1) {
			get_best_initial_tile(search_node_tile_pip, search_tile_tile_wire_pip, cti, tin, ustub, bestTI, bestDistance);
		}
		get_best_next_tile(search_tile_tile_wire_pip, cti, tin, ustub, tile_pips, bestTI, bestDistance);
		if (bestTI._value == INT32_MAX) {
			stub_router_count--;
			// puts(std::format("stub: {} stub_router_count:{} dist:{} {}:deadend", ustub->net_idx, stub_router_count, ustub->current_distance, tin.name).c_str());
			// stubs_deadend++;
			return;
		}

		if (bestDistance >= ustub->current_distance && ustub->tile_path.size() > 3) {
#if 1
			stub_router_count--;
			// stubs_further++;
			// puts(std::format("stub: {} current_dist:{} dist: {} dest:{} stub_router_count:{} further:{}", ustub->net_idx, ustub->current_distance, bestDistance, bestTI._value, stub_router_count, TileInfo::get_tile_path_str(ti, ustub)).c_str());
#else
			ustub->tile_path.pop_back();
			auto rb{ ustub->tile_path.rbegin() };
#endif
		}
		else if (bestDistance > 0.0) {
			ustub->current_distance = bestDistance;
			ustub->tile_path.emplace_back(bestTI._value);
			auto pos{ routed_index_count.fetch_add(2) };;
			if (pos + 2 < routed_indices.size()) {
				routed_indices[pos] = ustub->tile_path[ustub->tile_path.size() - 2]._value;
				routed_indices[pos + 1] = ustub->tile_path[ustub->tile_path.size() - 1]._value;
			}

			// puts(std::format("stub: {} current_dist:{} dist: {} dest:{} stub_router_count:{}", ustub->net_idx, ustub->current_distance, bestDistance, bestTI._value, stub_router_count).c_str());
			ti[bestTI._value].append_unhandled_out(ustub);
			// ti[bestTI._value].unhandled_out_nets.emplace_back(std::move(ustub));
		}
		else {
			ustub->current_distance = bestDistance;
			ustub->tile_path.emplace_back(bestTI._value);
			auto pos{ routed_index_count.fetch_add(2) };;
			if (pos + 2 < routed_indices.size()) {
				routed_indices[pos] = ustub->tile_path[ustub->tile_path.size() - 2]._value;
				routed_indices[pos + 1] = ustub->tile_path[ustub->tile_path.size() - 1]._value;
			}
			stub_router_count--;
			// stubs_finished++;
			// puts(std::format("stub: {} dist: {} dest:{} stub_router_count:{} finished", ustub->net_idx, bestDistance, bestTI._value, stub_router_count).c_str());
		}

	}

	constexpr static always_inline void route_tile(
		std::span<uint32_t> routed_indices,
		std::atomic<uint32_t>& routed_index_count,
		std::atomic<uint32_t>& unrouted_index_count,
		const Search_Node_Tile_Pip& search_node_tile_pip,
		const Search_Tile_Tile_Wire_Pip& search_tile_tile_wire_pip,
		std::span<const TileInfo, tile_count> cti,
		std::span<TileInfo, tile_count>& ti,
		TileInfo& tin,
		std::atomic<uint32_t>& stub_router_count
	)
	{
		if (!tin.handling_out_nets.size()) return;

		if (false)
			puts(std::format("{} handling_out_nets: {}, in_tiles: {}, out_tiles: {}",
				tin.name,
				tin.handling_out_nets.size(),
				tin.in_tiles.size(),
				tin.out_tiles.size()
			).c_str());

		auto ustubs{ std::move(tin.handling_out_nets) };
		auto tile_pips{ search_tile_tile_wire_pip.search_tile_tile_pip[tin.tile_idx._value] };
		for (auto ustub : ustubs) {
			route_tile_stub(
				routed_indices,
				routed_index_count,
				unrouted_index_count,
				search_node_tile_pip,
				search_tile_tile_wire_pip,
				cti,
				ti,
				tin,
				stub_router_count,
				tile_pips,
				ustub);
		}

	}

	constexpr static always_inline void move_unhandled_to_handled(
		uint64_t offset,
		uint64_t group_size,
		std::span<TileInfo, tile_count>& ti,
		std::atomic<uint32_t>& stubs_to_handle
	) {
		constexpr_each_n(offset, group_size, ti, [&](uint64_t tin_index, TileInfo& tin) {
			stubs_to_handle += static_cast<uint32_t>(tin.unhandled_out_nets.size());
			if (!tin.unhandled_out_nets.size()) return;
			// puts(std::format("{} unhandled_out_nets: {}", tin.name, tin.unhandled_out_nets.size()).c_str());
			tin.handle_out_nets();
			});
	}

	constexpr static always_inline void route_each_tile(
		std::span<uint32_t> routed_indices,
		std::atomic<uint32_t>& routed_index_count,
		std::atomic<uint32_t>& unrouted_index_count,
		const Search_Node_Tile_Pip& search_node_tile_pip,
		const Search_Tile_Tile_Wire_Pip& search_tile_tile_wire_pip,
		uint64_t offset,
		uint64_t group_size,
		std::span<const TileInfo, tile_count> cti,
		std::span<TileInfo, tile_count>& ti,
		std::atomic<uint32_t>& stub_router_count
	) {
		constexpr_each_n(offset, group_size, ti, [&](uint64_t tin_index, TileInfo& tin) {
			route_tile(
				routed_indices,
				routed_index_count,
				unrouted_index_count,
				search_node_tile_pip,
				search_tile_tile_wire_pip,
				cti,
				ti,
				tin,
				stub_router_count
			);
			});
	}

	static always_inline void route_tiles(
		std::span<uint32_t> routed_indices,
		std::atomic<uint32_t>& routed_index_count,
		std::atomic<uint32_t>& unrouted_index_count,
		const Search_Node_Tile_Pip& search_node_tile_pip,
		const Search_Tile_Tile_Wire_Pip& search_tile_tile_wire_pip,
		uint64_t offset,
		uint64_t group_size,
		std::span<TileInfo, tile_count>& ti,
		std::atomic<uint32_t>& stub_router_count,
		std::barrier<>& bar,
		std::atomic<uint32_t>& stubs_to_handle
	) {
		std::span<const TileInfo, tile_count> cti(ti.cbegin(), ti.size());

		for (;;) {
			bar.arrive_and_wait();
			if (!offset) stubs_to_handle.store(0);
			bar.arrive_and_wait();

			move_unhandled_to_handled(offset, group_size, ti, stubs_to_handle);

			bar.arrive_and_wait();
			// if (!offset) puts(std::format("stubs_to_handle: {}, stubs_further: {}, stubs_deadend: {}, stubs_finished: {}", stubs_to_handle.load(), stubs_further.load(), stubs_deadend.load(), stubs_finished.load()).c_str());

			route_each_tile(
				routed_indices,
				routed_index_count,
				unrouted_index_count,
				search_node_tile_pip,
				search_tile_tile_wire_pip,
				offset,
				group_size,
				cti,
				ti,
				stub_router_count
			);

			bar.arrive_and_wait();
			if (!stubs_to_handle) {
				break;
			}
		}


	}
};