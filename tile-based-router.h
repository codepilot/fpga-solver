#pragma once

#include "lib_dev_tile_index.h"
#include "lib_inverse_wires.h"
#include "lib_site_pin_to_wire.h"
#include "lib_site_pin_to_node.h"
#include "lib_wire_idx_to_tile_idx_tile_wire_idx.h"
#include <cstdlib>
#include <cmath>

template<class _Ty, size_t _Size>
class alignas(__m512i) aligned_array : public std::array<_Ty, _Size> { };

struct sv_u32 {
	std::string_view key;
	uint32_t value;
};

struct sv_u16v2 {
	std::string_view key;
	uint16_t x, y;
};

namespace xcvu3p {
	static std::unique_ptr<std::array<sv_u32, str_count>> make_strs_map() {
		auto ret{ std::make_unique<std::array<sv_u32, str_count>>() };
		std::span<sv_u32, str_count> s_ret{ ret->begin(), ret->end() };

		jthread_each<uint32_t>(strs, [&](uint32_t str_idx, capnp::Text::Reader dev_str) {
			std::string_view str{ dev_str.cStr() };
			s_ret[str_idx] = { str, str_idx };
		});

		std::sort(std::execution::par_unseq, s_ret.begin(), s_ret.end(), [](const sv_u32& a, const sv_u32& b)-> bool { return a.key < b.key; });

		return ret;
	}
	inline static const std::unique_ptr<std::array<sv_u32, str_count>> a_strs_map{ make_strs_map() };
	inline static const std::span<const sv_u32, str_count> s_strs_map{ a_strs_map->begin(), a_strs_map->end() };

	inline static constexpr size_t site_locations_size{ 82706ull };

	static std::unique_ptr<std::array<sv_u16v2, site_locations_size>> make_site_locations() {
		auto a_site_locations{ std::make_unique<std::array<sv_u16v2, site_locations_size>>() };
		std::span<sv_u16v2, site_locations_size> s_site_locations{ a_site_locations->begin(), a_site_locations->end() };
		std::atomic<uint32_t> site_location_offset{};

		jthread_each<uint32_t>(tiles, [&](uint32_t tile_idx, tile_reader tile) {
			auto sites{ tile.getSites() };
			auto n_site_location_offset{ site_location_offset.fetch_add(sites.size()) };
			std::array<uint16_t, 2> pos{ tile.getCol(), tile.getRow() };
			for (auto&& site : tile.getSites()) {
				s_site_locations[n_site_location_offset++] = sv_u16v2{ .key{strs[site.getName()].cStr()}, .x{pos[0]}, .y{pos[1]} };
			}
		});

		if (site_location_offset != site_locations_size) abort();

		std::sort(std::execution::par_unseq, s_site_locations.begin(), s_site_locations.end(), [](const sv_u16v2& a, const sv_u16v2& b)-> bool { return a.key < b.key; });

		return a_site_locations;
	}
	inline static const decltype(make_site_locations()) a_site_locations{ make_site_locations() };
	inline static const std::span<const sv_u16v2, site_locations_size> s_site_locations{ a_site_locations->begin(), a_site_locations->end() };

	struct PIP_SORTED_BY_WIRE0 {
		uint16_t wire1;
		uint16_t wire0;
	};

	struct PIP_SORTED_BY_WIRE1 {
		uint16_t wire0;
		uint16_t wire1;
	};

	using intra_tile_path = std::array<uint16_t, 32ull>;

	inline static constexpr size_t max_tile_wire_count{ 25285ull };
	inline static constexpr size_t max_intra_tile_path_count{ 25000000ull };

	static uint32_t count_tile_of_type(uint32_t tile_type_idx) noexcept {
		return std::count_if(tiles.begin(), tiles.end(), [tile_type_idx](tile_reader tile)-> bool { return tile.getType() == tile_type_idx;  });
	}

	struct PipsByWire {
		std::vector<PIP_SORTED_BY_WIRE0> pips_by_wire0;
		std::vector<PIP_SORTED_BY_WIRE1> pips_by_wire1;
		std::bitset<max_tile_wire_count> inbound_wires;
		std::bitset<max_tile_wire_count> outbound_wires;
	};

	static std::unique_ptr<PipsByWire> get_pips_by_wire(pip_list_reader pips) noexcept {
		auto ret{ std::make_unique<PipsByWire>() };
		size_t pip_count{ pips.size() };
		ret->pips_by_wire0.reserve(pip_count * 2ull);
		ret->pips_by_wire1.reserve(pip_count * 2ull);

		for(auto pip: pips) {
			const uint16_t tw0_idx{ static_cast<uint16_t>(pip.getWire0()) };
			const uint16_t tw1_idx{ static_cast<uint16_t>(pip.getWire1()) };

			ret->inbound_wires.set(tw0_idx);
			ret->outbound_wires.set(tw1_idx);
			ret->pips_by_wire0.emplace_back(PIP_SORTED_BY_WIRE0{ .wire1{tw1_idx}, .wire0{tw0_idx} });
			ret->pips_by_wire1.emplace_back(PIP_SORTED_BY_WIRE1{ .wire0{tw0_idx}, .wire1{tw1_idx} });

			if (!pip.getDirectional()) {
				ret->outbound_wires.set(tw0_idx);
				ret->inbound_wires.set(tw1_idx);
				ret->pips_by_wire0.emplace_back(PIP_SORTED_BY_WIRE0{ .wire1{tw0_idx}, .wire0{tw1_idx} });
				ret->pips_by_wire1.emplace_back(PIP_SORTED_BY_WIRE1{ .wire0{tw1_idx}, .wire1{tw0_idx} });
			}
		};

		std::sort(std::execution::par_unseq, ret->pips_by_wire0.begin(), ret->pips_by_wire0.end(), [](const PIP_SORTED_BY_WIRE0 a, const PIP_SORTED_BY_WIRE0 b)-> bool { return std::bit_cast<uint32_t>(a) < std::bit_cast<uint32_t>(b); });
		std::sort(std::execution::par_unseq, ret->pips_by_wire1.begin(), ret->pips_by_wire1.end(), [](const PIP_SORTED_BY_WIRE1 a, const PIP_SORTED_BY_WIRE1 b)-> bool { return std::bit_cast<uint32_t>(a) < std::bit_cast<uint32_t>(b); });
		return ret;
	}

	struct wire_pip_node {
		uint64_t wire_id;
		uint64_t parent;
		std::bitset<max_tile_wire_count> all_reachable_wires;
		std::bitset<max_tile_wire_count> head_reachable_wires;
		// std::vector<uint32_t> children;
		// std::set<uint16_t> all_reachable_wires;
		// std::set<uint16_t> head_reachable_wires;
	};

	static void build_tile_type_wire_pip_path(
		const uint64_t oi,
		const uint32_t tile_type_wire_name,
		uint32_list_reader tile_type_wires,
		const std::bitset<max_tile_wire_count>& outbound_only_wires,
		const std::bitset<max_tile_wire_count>& inbound_only_wires,
		const std::bitset<max_tile_wire_count>& bidirectional_wires,
		const size_t bidirectional_wire_count,
		const PipsByWire &pipsByWire,
		std::atomic<uint64_t> &total_ends,
		std::atomic<uint64_t> &max_depth,
		std::span<intra_tile_path> s_intra_tile_paths,
		std::atomic<uint64_t> &intra_tile_path_offset,
		std::atomic<uint64_t>& max_v_size
	) noexcept {
		if (!outbound_only_wires.test(oi)) return;
		std::vector<wire_pip_node> wpn;
		//				std::vector<wire_pip_end> wpn_ends;

		{
			std::bitset<max_tile_wire_count> init_reachable_wires;

			std::span<const PIP_SORTED_BY_WIRE1> init_pip_range{
				std::ranges::equal_range(
					pipsByWire.pips_by_wire1,
					PIP_SORTED_BY_WIRE1{.wire0{0}, .wire1{static_cast<uint16_t>(oi)} },
					[](const PIP_SORTED_BY_WIRE1 a, const PIP_SORTED_BY_WIRE1 b)-> bool { return a.wire1 < b.wire1; })
			};
			for (const PIP_SORTED_BY_WIRE1& pipn : init_pip_range) {
				init_reachable_wires.set(pipn.wire0);
			}

			wpn.emplace_back(wire_pip_node{
				.wire_id{oi},
				.parent{UINT64_MAX},
				.all_reachable_wires{init_reachable_wires},
				.head_reachable_wires{init_reachable_wires},
				});
		}

		size_t depth{};
		// std::set<std::set<uint64_t>> all_paths;
		std::map<uint16_t, std::unordered_set<std::bitset<max_tile_wire_count>>> map_all_paths;
		std::vector<uint64_t> ends;
		std::bitset<max_tile_wire_count> reachable_wires;
		for (size_t cn{}; cn < wpn.size(); cn++) {
			const wire_pip_node c_wpn{ wpn[cn] };
			if (inbound_only_wires.test(c_wpn.wire_id)) {
				continue;
			}
			// std::cout << std::format("{} ", c_wpn.wire_id);
			std::bitset<max_tile_wire_count> touched_wires;
			for (auto pt{ cn }; pt != UINT64_MAX; pt = wpn[pt].parent) {
				if (touched_wires.test(wpn[pt].wire_id)) abort();
				touched_wires.set(wpn[pt].wire_id);
			}
			decltype(auto) found_paths{ map_all_paths[c_wpn.wire_id] };
#if 0
			if (false)
				for (auto found_path : found_paths) {
					if (std::ranges::includes(touched_wires, found_path)) {
						std::vector<uint64_t> diff_a;
						std::vector<uint64_t> diff_b;

						std::ranges::set_difference(touched_wires, found_path, std::back_inserter(diff_a));
						std::ranges::set_difference(found_path, touched_wires, std::back_inserter(diff_b));
						std::cout << std::format("diffs: {}, {}\n", diff_a.size(), diff_b.size());
						abort();
					}
				}
#endif

			// all_paths.insert(touched_wires);
			found_paths.insert(touched_wires);

			if (c_wpn.head_reachable_wires.none()) abort();
			if (depth != touched_wires.count()) {
				if ((depth + 1) != touched_wires.count()) abort();
				depth = touched_wires.count();
				while (depth > max_depth.load()) {
					uint64_t current_max_depth{ max_depth.load() };
					if (depth > current_max_depth) {
						if (max_depth.compare_exchange_strong(current_max_depth, depth)) {
							std::cout << std::format("max_depth {} => {}\n", current_max_depth, depth);
						}
					}
				}
				// std::cout << std::format("  {}_{}_{}_{}_{}\n", depth, cn, wpn.size(), reachable_wires.size(), c_wpn.head_reachable_wires.size());
			}

			// if (touched_wires.count() >= 15) continue;

			for (uint16_t head_reachable_wire{}; head_reachable_wire < c_wpn.head_reachable_wires.size(); head_reachable_wire++) {
				if (!c_wpn.head_reachable_wires.test(head_reachable_wire)) continue;
				if (outbound_only_wires.test(head_reachable_wire)) {
					abort();
				}
				if (touched_wires.test(head_reachable_wire)) continue;

				if (inbound_only_wires.test(head_reachable_wire)) {
					//if (found_paths.contains(touched_wires)) {
					//	abort();
					//	continue;
					//}
					// all_paths.insert(touched_wires);
					// found_paths.insert(touched_wires);
					ends.emplace_back(cn);
					reachable_wires.set(head_reachable_wire);

					// wpn[cn].children.emplace_back(static_cast<uint32_t>(wpn.size()));
					wpn.emplace_back(wire_pip_node{
						.wire_id{ head_reachable_wire },
						.parent{ static_cast<uint64_t>(cn) },
						});
					continue;
				}

				std::span<const PIP_SORTED_BY_WIRE1> pip_range{
					std::ranges::equal_range(
						pipsByWire.pips_by_wire1,
						PIP_SORTED_BY_WIRE1{.wire0{0}, .wire1{static_cast<uint16_t>(head_reachable_wire)} },
						[](const PIP_SORTED_BY_WIRE1 a, const PIP_SORTED_BY_WIRE1 b)-> bool { return a.wire1 < b.wire1; }
					)
				};
				std::bitset<max_tile_wire_count> n_reachable{ c_wpn.all_reachable_wires };
				std::bitset<max_tile_wire_count> n_head_reachable;
				bool added_reachable{ false };
				for (const PIP_SORTED_BY_WIRE1& pipn : pip_range) {
					//auto [_, is_added] = n_reachable.insert(pipn.wire0);
					//added_reachable = added_reachable || is_added;

					if (!n_reachable.test(pipn.wire0)) {
						n_head_reachable.set(pipn.wire0);
						n_reachable.set(pipn.wire0);
						added_reachable = true;
					}
				}
				//if (std::ranges::includes(c_wpn.reachable_wires, n_reachable)) {
				//	continue;
				//}
				if ((!added_reachable) && bidirectional_wires.test(head_reachable_wire)) {
					continue;
				}

				// wpn[cn].children.emplace_back(static_cast<uint32_t>(wpn.size()));

				wpn.emplace_back(wire_pip_node{
					.wire_id{ head_reachable_wire },
					.parent{ static_cast<uint64_t>(cn) },
					.all_reachable_wires{std::move(n_reachable)},
					.head_reachable_wires{std::move(n_head_reachable)},
					});
			}
		}
		auto my_total_ends{ total_ends.fetch_add(ends.size()) };
		if (depth > 10)
			std::cout << std::format("total_ends:{:6.2f}M {} bidi:{} depth:{} wpn:{} ends:{} reachable:{}\n",
				std::scalbln(static_cast<double>(my_total_ends), -20), strs[tile_type_wires[oi]].cStr(),
				bidirectional_wire_count,
				depth, wpn.size(), ends.size(), reachable_wires.count()
			);

		for (uint64_t end_idx : ends) {
			std::array<uint16_t, 31> wire_path;
			wire_path.fill(UINT16_MAX);
			size_t wire_path_offset{};
			for (auto pt{ end_idx }; pt != UINT64_MAX; pt = wpn[pt].parent) {
				if (wire_path_offset >= wire_path.size()) {
					abort();
				}
				wire_path[wire_path_offset] = wpn[pt].wire_id;
				++wire_path_offset;
			}
			std::span<uint16_t> s_wire_path{ std::span(wire_path).first(wire_path_offset) };
			auto last_wire_idx{ s_wire_path.back() };
			auto first_wire_idx{ s_wire_path.front() };
			std::span<uint16_t> s_wire_path_skip_last{ s_wire_path.subspan(1, s_wire_path.size() - 2) };
			std::vector<uint16_t> v_intra_tile_path;
			v_intra_tile_path.reserve(32);
			v_intra_tile_path.emplace_back(last_wire_idx);
			v_intra_tile_path.emplace_back(first_wire_idx);
			v_intra_tile_path.emplace_back(static_cast<uint16_t>(s_wire_path_skip_last.size()));
			std::copy(s_wire_path_skip_last.rbegin(), s_wire_path_skip_last.rend(), std::back_inserter(v_intra_tile_path));
			v_intra_tile_path.resize(32, UINT16_MAX);
			intra_tile_path a_path;
			std::ranges::copy(v_intra_tile_path, a_path.begin());
			s_intra_tile_paths[intra_tile_path_offset++] = a_path;
			//std::for_each(wire_path.rbegin(), wire_path.rend(), [](uint16_t wire_idx) {
			//	std::cout << std::format("{} ", wire_idx);
			//});
			//std::cout << "\n";
		}
		// total_ends += wpn_ends.size();
//				std::cout << "\n\n";
		if (wpn.size() > max_v_size) {
			max_v_size = wpn.size();
			std::cout << std::format("wpn.size: {}\n", wpn.size());
		}
	}

	static void enum_tile_paths(
		uint32_t tile_type_idx,
		tile_type_reader tile_type,
		std::atomic<uint64_t> &intra_tile_path_offset,
		std::atomic<uint64_t> &max_depth,
		std::atomic<uint64_t> &total_ends,
		std::span<intra_tile_path> s_intra_tile_paths,
		std::atomic<uint64_t> &max_v_size
	) noexcept {
		std::string_view tile_type_name{ strs[tile_type.getName()].cStr() };

		auto tile_type_wires{ tile_type.getWires() };
		auto tile_type_wire_count{ tile_type_wires.size() };
		auto pips{ tile_type.getPips() };
		if (!pips.size()) return;

		auto up_valid_wires{ std::make_unique<std::bitset<max_tile_wire_count>>() };
		auto up_bidirectional_wires{ std::make_unique<std::bitset<max_tile_wire_count>>() };
		auto up_passthru_wires{ std::make_unique<std::bitset<max_tile_wire_count>>() };
		auto up_monodirectional_wires{ std::make_unique<std::bitset<max_tile_wire_count>>() };
		auto up_inbound_only_wires{ std::make_unique<std::bitset<max_tile_wire_count>>() };
		auto up_outbound_only_wires{ std::make_unique<std::bitset<max_tile_wire_count>>() };

		
		auto pipsByWire{ get_pips_by_wire(pips) };

		decltype(auto) valid_wires{ *up_valid_wires.get() };
		decltype(auto) bidirectional_wires{ *up_bidirectional_wires.get() };
		decltype(auto) passthru_wires{ *up_passthru_wires.get() };
		decltype(auto) monodirectional_wires{ *up_monodirectional_wires.get() };
		decltype(auto) inbound_only_wires{ *up_inbound_only_wires.get() };
		decltype(auto) outbound_only_wires{ *up_outbound_only_wires.get() };

		for (uint32_t i{}; i < tile_type_wire_count; i++) valid_wires.set(i);
		const size_t valid_wire_count{ valid_wires.count() };
		if (valid_wire_count != tile_type_wire_count) {
			abort();
		}

		bidirectional_wires = pipsByWire->inbound_wires & pipsByWire->outbound_wires;
		passthru_wires = valid_wires ^ (pipsByWire->inbound_wires | pipsByWire->outbound_wires);
		monodirectional_wires = pipsByWire->inbound_wires ^ pipsByWire->outbound_wires;

		inbound_only_wires = pipsByWire->inbound_wires & monodirectional_wires;
		outbound_only_wires = pipsByWire->outbound_wires & monodirectional_wires;

		const size_t inbound_only_wire_count{ inbound_only_wires.count() };
		const size_t outbound_only_wire_count{ outbound_only_wires.count() };
		const size_t bidirectional_wire_count{ bidirectional_wires.count() };
		const size_t passthru_wire_count{ passthru_wires.count() };
		const size_t sum_categories{ inbound_only_wire_count + outbound_only_wire_count + bidirectional_wire_count + passthru_wire_count };
		if (sum_categories != tile_type_wire_count) {
			abort();
		}
		if (!inbound_only_wire_count) return;
		if (!outbound_only_wire_count) return;
		if (!bidirectional_wire_count) return;

		std::cout << std::format("{}x {} in: {} out: {} bidi: {}\n",
			count_tile_of_type(tile_type_idx),
			tile_type_name,
			inbound_only_wire_count,
			outbound_only_wire_count,
			bidirectional_wire_count
		);


#if 0
		struct wire_pip_end {
			uint64_t wire_id;
			uint64_t parent;
		};
#endif

		const auto intra_tile_path_offset_start{ intra_tile_path_offset.load() };

		for (uint64_t oi = 0; oi < tile_type_wires.size(); oi++) {
			build_tile_type_wire_pip_path(oi, tile_type_wires[oi],
				tile_type_wires,
				outbound_only_wires,
				inbound_only_wires,
				bidirectional_wires,
				bidirectional_wire_count,
				*pipsByWire,
				total_ends,
				max_depth,
				s_intra_tile_paths,
				intra_tile_path_offset,
				max_v_size
			);
		}

		const auto intra_tile_path_offset_count{ intra_tile_path_offset.load() - intra_tile_path_offset_start };
		std::span<intra_tile_path> tile_paths{ s_intra_tile_paths.subspan(intra_tile_path_offset_start, intra_tile_path_offset_count) };
		std::cout << std::format("sorting {} paths... ", tile_paths.size());
		std::sort(std::execution::par_unseq, tile_paths.begin(), tile_paths.end());
		std::cout << "done\n";

		std::cout << std::format("\n\n");
	}

	static auto make_pip_paths() {
		std::atomic<uint64_t> total_ends;
		std::atomic<uint64_t> max_depth;
		std::atomic<uint64_t> max_v_size;

		std::vector<intra_tile_path> v_intra_tile_paths(max_intra_tile_path_count);
		std::atomic<uint64_t> intra_tile_path_offset;

		for (uint32_t tile_type_idx = 0; tile_type_idx < tileTypes.size(); ++tile_type_idx) {
			enum_tile_paths(tile_type_idx, tileTypes[tile_type_idx], intra_tile_path_offset, max_depth, total_ends, v_intra_tile_paths, max_v_size);
		}

		std::cout << std::format("\n\ntotal_ends: {}, max_depth: {}\n", total_ends.load(), max_depth.load());
		auto s_intra_tile_paths{ std::span(v_intra_tile_paths).first(intra_tile_path_offset.load()) };
		std::cout << "writing file... ";
		{
			MemoryMappedFile mmf_intra_tile_paths{ "intra_tile_paths.bin", s_intra_tile_paths.size_bytes() };
			auto s_mmf_intra_tile_paths{ mmf_intra_tile_paths.get_span<intra_tile_path>() };
			std::ranges::copy(s_intra_tile_paths, s_mmf_intra_tile_paths.begin());
		}
		std::cout << "done\n";
#if 0
		for (auto& intra_tile_path : std::span(v_intra_tile_paths).first(intra_tile_path_offset.load())) {
			std::cout << std::format("front:{} back:{} size:{}\n", intra_tile_path[0], intra_tile_path[1], intra_tile_path[2]);
		}
#endif
		return true;
	}
	inline static const decltype(make_pip_paths()) a_pip_paths{ make_pip_paths() };

};

class Tile_Based_Router {
public:

	static inline constexpr size_t bans_per_tile{ 100ull };
	static inline constexpr size_t shader_workgroup_size{ 256ull };
	static inline constexpr size_t tile_shader_count{ ((xcvu3p::tile_count + (shader_workgroup_size - 1ull)) / shader_workgroup_size) * shader_workgroup_size };
	static inline constexpr size_t total_ban_count{ bans_per_tile * tile_shader_count };

	phys_reader& phys;
	net_list_reader nets;

//host side
	const std::vector<uint32_t> v_physStrs_to_devStrs;
	const std::span<const uint32_t> s_physStrs_to_devStrs;

//	std::unique_ptr<aligned_array<uint32_t, xcvu3p::str_count>> a_devStrs_to_physStrs;
//	std::span<uint32_t, xcvu3p::str_count> s_devStrs_to_physStrs;

//	std::unique_ptr<aligned_array<uint16_t, xcvu3p::wire_count>> a_inbox_modified_wires;
//	std::unique_ptr<aligned_array<uint32_t, total_ban_count>> a_ban_net_ids;
//	std::unique_ptr<aligned_array<uint16_t, total_ban_count>> a_ban_tile_wire_ids;

//shader side
//	std::span<uint32_t, xcvu3p::wire_count> s_tile_wire_net_id;
//	std::span<uint32_t, xcvu3p::wire_count> s_tile_wire_previous_tile_id;
//	std::span<uint16_t, xcvu3p::wire_count> s_tile_wire_previous_tile_wire;
//	std::span<uint16_t, xcvu3p::wire_count> s_inbox_modified_wires;

//	std::span<uint32_t, total_ban_count> s_ban_net_ids;
//	std::span<uint16_t, total_ban_count> s_ban_tile_wire_ids;

//	std::span<const uint32_t, tile_shader_count> s_tile_wire_offset; //readonly, one per tile
//	std::span<const uint16_t, tile_shader_count> s_tile_wire_count; //readonly, one per tile

//	std::span<const uint32_t, tile_shader_count> s_inbox_offset; //readonly, one per tile
//	std::span<const uint16_t, tile_shader_count> s_inbox_count; //readonly, one per tile

//	std::span<const uint32_t, tile_shader_count> s_ban_offset; //readonly, one per tile
//	std::span<const uint16_t, tile_shader_count> s_ban_count; //readonly, one per tile

//	std::span<const std::array<const uint16_t, 2>, tile_shader_count> tile_pos; //readonly
//	std::span<uint16_t, tile_shader_count> inbox_read_pos; //written by each shader
//	std::span<const uint16_t, tile_shader_count> inbox_previously_read_pos; //readonly
//	std::span<const uint16_t, tile_shader_count> inbox_written_pos; //readonly
//	std::span<std::atomic<uint16_t>, tile_shader_count> inbox_write_pos; //atomic increment

	Tile_Based_Router() = delete;
	Tile_Based_Router(Tile_Based_Router& other) = delete;
	Tile_Based_Router& operator=(Tile_Based_Router& other) = delete;
	Tile_Based_Router operator=(Tile_Based_Router other) = delete;
	Tile_Based_Router(Tile_Based_Router&& other) = delete;
	Tile_Based_Router& operator=(Tile_Based_Router&& other) = delete;


	static std::tuple<std::vector<uint32_t>, std::vector<uint32_t>> make_string_interchange(const std::unordered_map<std::string_view, uint32_t>& devStrs_map, ::capnp::List< ::capnp::Text, ::capnp::Kind::BLOB>::Reader devStrs, ::capnp::List< ::capnp::Text, ::capnp::Kind::BLOB>::Reader physStrs) {
		std::vector<uint32_t> physStrs_to_devStrs(static_cast<size_t>(physStrs.size()), UINT32_MAX);
		std::vector<uint32_t> devStrs_to_physStrs(static_cast<size_t>(devStrs.size()), UINT32_MAX);
		each<uint32_t>(physStrs, [&](uint32_t phys_strIdx, capnp::Text::Reader phys_str) {
			std::string_view str{ phys_str.cStr() };
			if (!devStrs_map.contains(str)) return;
			auto dev_strIdx{ devStrs_map.at(str) };
			physStrs_to_devStrs[phys_strIdx] = dev_strIdx;
			devStrs_to_physStrs[dev_strIdx] = phys_strIdx;
			});
		return { physStrs_to_devStrs , devStrs_to_physStrs };
	}

	static std::vector<uint32_t> make_physStrs_to_devStrs(string_list_reader physStrs) {
		std::vector<uint32_t> physStrs_to_devStrs(static_cast<size_t>(physStrs.size()), UINT32_MAX);
		each<uint32_t>(physStrs, [&](uint32_t phys_strIdx, capnp::Text::Reader phys_str) {
			std::string_view str{ phys_str.cStr() };
			auto found{ std::ranges::equal_range(xcvu3p::s_strs_map, sv_u32{ str, UINT32_MAX }, [](const sv_u32& a, const sv_u32& b)-> bool { return a.key < b.key; }) };
			if (found.empty()) return;
			if (found.size() != 1) abort();
			physStrs_to_devStrs[phys_strIdx] = found.front().value;
		});
		return physStrs_to_devStrs;
	}

	Tile_Based_Router(
		phys_reader &_phys
	) noexcept :

		phys{ _phys },
		nets{ phys.getPhysNets() },
		v_physStrs_to_devStrs{ make_physStrs_to_devStrs(phys.getStrList()) },
		s_physStrs_to_devStrs{ v_physStrs_to_devStrs }
	{
	}

#if 0
	auto get_shader_tile_wire(uint32_t wire_idx) noexcept-> Tile_Wire& {
		decltype(auto) tw{ xcvu3p::wire_idx_to_tile_idx_tile_wire_idx[wire_idx] };
		decltype(auto) tile_info{ s_tile_infos[tw.tile_idx] };
		auto stw{ s_tile_wires.subspan(tile_info.tile_wire_offset, tile_info.tile_wire_count) };
		return stw[tw.tile_wire_idx];
	}

	void set_shader_tile_wire(uint32_t wire_idx, Tile_Wire&& ntw) noexcept {
		get_shader_tile_wire(wire_idx) = ntw;
	}

	void set_shader_tile_wire_inbox(uint32_t wire_idx, Tile_Wire&& ntw) noexcept {
		decltype(auto) tw{ xcvu3p::wire_idx_to_tile_idx_tile_wire_idx[wire_idx] };
		decltype(auto) tile_info{ s_tile_infos[tw.tile_idx] };
		auto stw{ s_tile_wires.subspan(tile_info.tile_wire_offset, tile_info.tile_wire_count) };
		stw[tw.tile_wire_idx] = ntw;
		auto sti{ s_inbox_items.subspan(tile_info.inbox_offset, tile_info.inbox_count) };
		auto inbox_write_pos{ tile_info.inbox_write_pos++ };
		sti[inbox_write_pos].modified_wire = tw.tile_wire_idx;
	}
#endif

	template<bool is_source>
	void block_site_pin(uint32_t net_idx, phys_site_pin_reader sitePin) {
		const auto ps_site{ sitePin.getSite() };
		const auto ps_pin{ sitePin.getPin() };

		const auto ds_site{ s_physStrs_to_devStrs[ps_site] };
		const auto ds_pin{ s_physStrs_to_devStrs[ps_pin] };

		const auto v_node_idx{ xcvu3p::site_pin_to_node.at(ds_site, ds_pin) };
		if (v_node_idx.empty()) abort();
		if (v_node_idx.size() != 1) abort();
		const auto node_idx{ v_node_idx.front() };

		const auto v_wire_idx{ xcvu3p::site_pin_to_wire.at(ds_site, ds_pin) };
		if (v_wire_idx.empty()) abort();
		if (v_wire_idx.size() != 1) abort();
		const auto wire_idx{ v_wire_idx.front() };

		decltype(auto) wire_tw{ xcvu3p::wire_idx_to_tile_idx_tile_wire_idx[wire_idx] };

		const auto node_wires{ xcvu3p::nodes[node_idx].getWires()};
		if (!node_wires.size()) abort();

		if (is_source) {
			for (auto node_wire_idx : node_wires) {
				// set_shader_tile_wire(node_wire_idx, { .net_id{net_idx}, .previous_tile_id{UINT32_MAX}, .previous_tile_wire{UINT16_MAX} });
			}
		} else {
			// set_shader_tile_wire_inbox(wire_idx, { .net_id{net_idx}, .previous_tile_id{UINT32_MAX}, .previous_tile_wire{UINT16_MAX} });
		}
	}

	template<bool is_source>
	void block_pip(uint32_t net_idx, ::PhysicalNetlist::PhysNetlist::PhysPIP::Reader pip) {
		auto ps_tile{ pip.getTile() };
		auto ps_wire0{ pip.getWire0() };
		auto ps_wire1{ pip.getWire1() };

		auto ds_tile{ s_physStrs_to_devStrs[ps_tile] };
		auto ds_wire0{ s_physStrs_to_devStrs[ps_wire0] };
		auto ds_wire1{ s_physStrs_to_devStrs[ps_wire1] };

		auto wire0_idx{ xcvu3p::inverse_wires.at(ds_tile, ds_wire0) };
		auto wire1_idx{ xcvu3p::inverse_wires.at(ds_tile, ds_wire1) };

		if (!wire0_idx.empty()) {
			auto node0_idx{ xcvu3p::wire_idx_to_node_idx[wire0_idx.at(0)] };
#if 0
			s_node_nets[node0_idx] = net_idx;
#endif
		}
		else {
			abort();
		}
		if (!wire1_idx.empty()) {
			auto node1_idx{ xcvu3p::wire_idx_to_node_idx[wire1_idx.at(0)] };
#if 0
			s_node_nets[node1_idx] = net_idx;
#endif
		}
		else {
			abort();
		}
	}

	template<bool is_source>
	void block_resource(uint32_t net_idx, PhysicalNetlist::PhysNetlist::RouteBranch::Reader branch) {
		auto rs{ branch.getRouteSegment() };
		switch (rs.which()) {
		case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::Which::BEL_PIN: {
			auto belPin{ rs.getBelPin() };
			// block_bel_pin(net_idx, belPin);
			break;
		}
		case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::Which::SITE_PIN: {
			auto sitePin{ rs.getSitePin() };
			block_site_pin<is_source>(net_idx, sitePin);
			break;
		}
		case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::Which::PIP: {
			auto pip{ rs.getPip() };
			block_pip<is_source>(net_idx, pip);
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
			block_resource<is_source>(net_idx, sub_branch);
		}
	}

	void block_resources(uint32_t net_idx, net_reader physNet) {
		for (auto&& src_branch : physNet.getSources()) {
			block_resource<true>(net_idx, src_branch);
		}
		for (auto&& stub_branch : physNet.getStubs()) {
			block_resource<false>(net_idx, stub_branch);
		}
	}

	bool block_all_resources() {
		jthread_each<uint32_t>(nets, [&](uint32_t net_idx, net_reader net) {
			block_resources(net_idx, net);
		});

		return true;
	}

#if 0
	static Tile_Based_Router make(phys_reader &phys) {
		std::atomic<uint32_t> needed_tile_wires, needed_inbox_items, needed_bans;
		jthread_each<uint32_t>(xcvu3p::tiles, [&](uint32_t tile_idx, tile_reader tile) {
			auto tile_type{ xcvu3p::tileTypes[tile.getType()] };
			auto tile_wires{ tile_type.getWires() };
			auto tile_pips{ tile_type.getPips() };

			needed_tile_wires += tile_wires.size();// fixme only need inbound wires
			needed_inbox_items += tile_wires.size();// fixme only need inbound wires
			needed_bans += bans_per_tile; //fixme find what works, runs out, etc
		});

		auto tile_infos{ std::vector<Tile_Info>(xcvu3p::tile_count, Tile_Info{}) };
		auto tile_wires{ std::vector<Tile_Wire>(static_cast<size_t>(needed_tile_wires.load()), Tile_Wire{}) };
		auto inbox_items{ std::vector<Inbox_Item>(static_cast<size_t>(needed_inbox_items.load()), Inbox_Item{}) };
		auto bans{ std::vector<Ban>(static_cast<size_t>(needed_bans.load()), Ban{}) };

		std::atomic<uint32_t> offset_tile_wires, offset_inbox_items, offset_bans;
		jthread_each<uint32_t>(xcvu3p::tiles, [&](uint32_t tile_idx, tile_reader tile) {
			auto tile_type{ xcvu3p::tileTypes[tile.getType()] };
			auto tile_wires{ tile_type.getWires() };
			auto tile_pips{ tile_type.getPips() };

			Tile_Info &tile_info{ tile_infos[tile_idx] = Tile_Info::make(
				tile_idx,
				offset_tile_wires, offset_inbox_items, offset_bans,
				tile_wires.size(),
				tile_wires.size(),
				bans_per_tile
			)};
		});

		auto site_locations{ TimerVal(make_site_locations(xcvu3p::root)) };
		const auto devStrs_map{ TimerVal(make_devStrs_map(xcvu3p::root)) };

		string_list_reader physStrs{ phys.getStrList() };
		auto [physStrs_to_devStrs, devStrs_to_physStrs] = TimerVal(make_string_interchange(devStrs_map, xcvu3p::strs, physStrs));

		std::cout << std::format("sizeof(Tile_Based_Router::Tile_Info) {}\n", sizeof(Tile_Based_Router::Tile_Info));

		return Tile_Based_Router(
		//host side
			std::move(physStrs_to_devStrs),
			std::move(devStrs_to_physStrs),
		//shader side
			std::move(tile_infos),
			std::move(tile_wires),
			std::move(inbox_items),
			std::move(bans),
			phys
		);
	}
#endif

	bool route_step() {
#if 0
		std::for_each(std::execution::par_unseq, s_tile_infos.begin(), s_tile_infos.end(), [&](Tile_Info& tile_info) {
			tile_info.each(s_inbox_items, s_tile_wires, [&](Tile_Wire &wire) {
				std::cout << std::format("{} {} {} {}\n", tile_info.tile_id, wire.net_id, wire.previous_tile_id, wire.previous_tile_wire);
			});
		});
#endif
		return true;
	}
};

static_assert(alignof(aligned_array<uint32_t, 1024>) == alignof(__m512i));
