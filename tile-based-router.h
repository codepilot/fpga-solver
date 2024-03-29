#pragma once

#include "lib_dev_tile_index.h"
#include "lib_inverse_wires.h"
#include "lib_site_pin_to_wire.h"
#include "lib_site_pin_to_node.h"
#include "lib_wire_idx_to_tile_idx_tile_wire_idx.h"
#include <cstdlib>
#include <cmath>
#include <thread>
#include <nmmintrin.h>

#ifdef _MSC_VER
#include <intrin.h>
#endif

#include <map>

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

	class flat_span {
	public:
		uint16_t count;
		int32_t index;
		inline constexpr bool empty() const noexcept { return !count; }
		inline constexpr size_t size() const noexcept { return count; }
	};

	class SpanIndexCount {
	public:
		std::span<int32_t> s_u32_index;
		std::span<uint16_t> s_u16_count;
		inline static SpanIndexCount make(const MemoryMappedFile& mmf_u32_index, const MemoryMappedFile& mmf_u16_count) {
			SpanIndexCount ret{
				.s_u32_index{mmf_u32_index.get_span<int32_t>()},
				.s_u16_count{mmf_u16_count.get_span<uint16_t>()},
			};
			if (ret.s_u32_index.size() != ret.s_u16_count.size()) abort();
			return ret;
		}
		inline size_t size() const noexcept { return s_u32_index.size(); }
		inline bool empty() const noexcept { return !size(); }
	};

	class FlatPipPaths {
	public:

		const MemoryMappedFile mmf_flat_tree_u32_index{ "flat_tree_u32_index.bin" };
		const MemoryMappedFile mmf_flat_tree_u16_count{ "flat_tree_u16_count.bin" };
		const SpanIndexCount s_flat_tree{ SpanIndexCount::make(mmf_flat_tree_u32_index, mmf_flat_tree_u16_count) };


		const MemoryMappedFile mmf_flat_tile_u32_index{ "flat_tile_u32_index.bin" };
		const MemoryMappedFile mmf_flat_tile_u16_count{ "flat_tile_u16_count.bin" };
		const SpanIndexCount s_flat_tile{ SpanIndexCount::make(mmf_flat_tile_u32_index, mmf_flat_tile_u16_count) };

		const MemoryMappedFile mmf_flat_start_u32_index{ "flat_start_u32_index.bin" };
		const MemoryMappedFile mmf_flat_start_u16_count{ "flat_start_u16_count.bin" };
		const SpanIndexCount s_flat_start{ SpanIndexCount::make(mmf_flat_start_u32_index, mmf_flat_start_u16_count) };

		const MemoryMappedFile mmf_flat_finish_u32_index{ "flat_finish_u32_index.bin" };
		const MemoryMappedFile mmf_flat_finish_u16_count{ "flat_finish_u16_count.bin" };
		const SpanIndexCount s_flat_finish{ SpanIndexCount::make(mmf_flat_finish_u32_index, mmf_flat_finish_u16_count) };

		const MemoryMappedFile mmf_flat_depth_u32_index{ "flat_depth_u32_index.bin" };
		const std::span<uint32_t> s_flat_depth_u32_index{ mmf_flat_depth_u32_index.get_span<uint32_t>() };

		const MemoryMappedFile mmf_flat_path_u32_index{ "flat_path_u32_index.bin" };
		const MemoryMappedFile mmf_flat_path_u16_count{ "flat_path_u16_count.bin" };
		const SpanIndexCount s_flat_path{ SpanIndexCount::make(mmf_flat_path_u32_index, mmf_flat_path_u16_count) };

		const MemoryMappedFile mmf_flat_wires_u16_index{ "flat_wires_u16_index.bin" };
		const std::span<uint16_t> s_flat_wires_u16_index{ mmf_flat_wires_u16_index.get_span<uint16_t>() };

		class Subscript1 : public flat_span { };
		class Subscript2 : public flat_span { };
		class Subscript3 : public flat_span { };
		class Subscript4 : public flat_span { };
		class Subscript5 : public flat_span { };

		inline Subscript1 subscript(uint16_t tileTypeIdx) const noexcept {
			const Subscript1 result0{ flat_span{.count{s_flat_tree.s_u16_count[tileTypeIdx]}, .index{s_flat_tree.s_u32_index[tileTypeIdx]}} };

			return result0;
		}

		inline Subscript2 subscript(uint16_t tileTypeIdx, uint16_t startWireIdx) const noexcept {
			const auto result0{ subscript(tileTypeIdx) };

			int32_t off_startWireIdx{ result0.index + static_cast<int32_t>(startWireIdx) };
			const Subscript2 result1{ flat_span{.count{s_flat_tile.s_u16_count[off_startWireIdx]}, .index{s_flat_tile.s_u32_index[off_startWireIdx]}} };

			return result1;
		}

		inline Subscript2 subscript(tile_idx_tile_wire_idx tw) const noexcept {
			return subscript(xcvu3p::tiles[tw.tile_idx].getType(), tw.tile_wire_idx);
		}

		inline Subscript2 subscript_by_wireIdx(uint32_t wireIdx) const noexcept {
			return subscript(xcvu3p::wire_idx_to_tile_idx_tile_wire_idx[wireIdx]);
		}

		inline Subscript3 subscript(Subscript2 result1, uint16_t finishWireIdx) const noexcept {
			int32_t off_finishWireIdx{ result1.index + static_cast<int32_t>(finishWireIdx) };
			const Subscript3 result2{ flat_span{.count{s_flat_start.s_u16_count[off_finishWireIdx]}, .index{s_flat_start.s_u32_index[off_finishWireIdx]}} };

			return result2;
		}

		inline Subscript3 subscript(uint16_t tileTypeIdx, uint16_t startWireIdx, uint16_t finishWireIdx) const noexcept {
			const Subscript2 result1{ subscript(tileTypeIdx, startWireIdx) };

			return subscript(result1, finishWireIdx);
		}

		inline Subscript4 subscript(Subscript3 result2, uint16_t depth) const noexcept {
			int32_t off_depth{ result2.index + static_cast<int32_t>(depth) };
			const Subscript4 result3{ flat_span{.count{s_flat_finish.s_u16_count[off_depth]}, .index{s_flat_finish.s_u32_index[off_depth]}} };

			return result3;
		}

		inline Subscript4 subscript(uint16_t tileTypeIdx, uint16_t startWireIdx, uint16_t finishWireIdx, uint16_t depth) const noexcept {
			const Subscript3 result2{ subscript(tileTypeIdx, startWireIdx, finishWireIdx) };

			return subscript(result2, depth);
		}

		inline std::span<uint32_t> subscript(const Subscript4 result3) const noexcept {
			const std::span<uint32_t> result4{ s_flat_depth_u32_index.subspan(result3.index, result3.count) };
			return result4;
		}

		inline std::span<uint16_t> subscript(const Subscript4 result3, uint16_t path_idx) const noexcept {
			const std::span<uint32_t> result4{ s_flat_depth_u32_index.subspan(result3.index, result3.count) };

			const auto result5{ result4[path_idx] };

			const flat_span result6{ .count{ s_flat_path.s_u16_count[result5] }, .index{ s_flat_path.s_u32_index[result5] } };

			const auto result7{ s_flat_wires_u16_index.subspan(result6.index, result6.count) };

			return result7;
		}

		inline uint32_t subscript_path_id(const Subscript4 result3, uint16_t path_idx) const noexcept {
			const std::span<uint32_t> result4{ s_flat_depth_u32_index.subspan(result3.index, result3.count) };

			const auto result5{ result4[path_idx] };

			return result5;
		}

		inline std::span<uint16_t> subscript(uint16_t tileTypeIdx, uint16_t startWireIdx, uint16_t finishWireIdx, uint16_t depth, uint16_t path_idx) const noexcept {
			const Subscript4 result3{ subscript(tileTypeIdx, startWireIdx, finishWireIdx, depth) };

			return subscript(result3, path_idx);
		}

	};

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

	struct wire_pip_node_small {
		uint32_t parent;
		uint16_t wire_id;
	};

	struct wire_pip_node {
		std::bitset<max_tile_wire_count> all_reachable_wires;
		std::bitset<max_tile_wire_count> head_reachable_wires;
		uint32_t parent;
		uint16_t wire_id;
		// std::vector<uint32_t> children;
		// std::set<uint16_t> all_reachable_wires;
		// std::set<uint16_t> head_reachable_wires;
	};

	static std::unique_ptr<wire_pip_node> make_init_wire_pip_node(const uint16_t start_wire_index, const PipsByWire& pipsByWire) noexcept {
		std::bitset<max_tile_wire_count> init_reachable_wires;

		std::span<const PIP_SORTED_BY_WIRE1> init_pip_range{
			std::ranges::equal_range(
				pipsByWire.pips_by_wire1,
				PIP_SORTED_BY_WIRE1{.wire0{0}, .wire1{start_wire_index} },
				[](const PIP_SORTED_BY_WIRE1 a, const PIP_SORTED_BY_WIRE1 b)-> bool { return a.wire1 < b.wire1; })
		};

		for (const PIP_SORTED_BY_WIRE1& pipn : init_pip_range) {
			init_reachable_wires.set(pipn.wire0);
		}

		return std::make_unique<wire_pip_node>(wire_pip_node{
			.all_reachable_wires{init_reachable_wires},
			.head_reachable_wires{init_reachable_wires},
			.parent{UINT32_MAX},
			.wire_id{start_wire_index},
		});
	}

	static std::bitset<max_tile_wire_count> get_touched_wires(size_t cn, std::span<wire_pip_node_small> s_wpns) noexcept {
		std::bitset<max_tile_wire_count> touched_wires;
		for (auto pt{ cn }; pt != UINT32_MAX; pt = s_wpns[pt].parent) {
			if (touched_wires.test(s_wpns[pt].wire_id)) abort();
			touched_wires.set(s_wpns[pt].wire_id);
		}
		return touched_wires;
	}

	static std::unique_ptr<wire_pip_node> get_reachable_wire(
		const uint32_t head_index,
		const uint16_t head_reachable_wire,
		const wire_pip_node &c_wpn,
		const std::bitset<max_tile_wire_count>& inbound_only_wires,
		const std::bitset<max_tile_wire_count>& outbound_only_wires,
		const std::bitset<max_tile_wire_count>& bidirectional_wires,
		const std::bitset<max_tile_wire_count>& touched_wires,
		const PipsByWire& pipsByWire,
		//std::vector<uint64_t>& ends,
		std::bitset<max_tile_wire_count>& reachable_wires
	) noexcept {
		if (!c_wpn.head_reachable_wires.test(head_reachable_wire)) return {};
		if (outbound_only_wires.test(head_reachable_wire)) {
			abort();
		}
		if (touched_wires.test(head_reachable_wire)) return {};

		if (inbound_only_wires.test(head_reachable_wire)) {
			//if (found_paths.contains(touched_wires)) {
			//	abort();
			//	continue;
			//}
			// all_paths.insert(touched_wires);
			// found_paths.insert(touched_wires);
			// ends.emplace_back(cn);
			reachable_wires.set(head_reachable_wire);

			// wpn[cn].children.emplace_back(static_cast<uint32_t>(wpn.size()));
			return std::make_unique<wire_pip_node>(wire_pip_node{
				.parent{ head_index },
				.wire_id{ head_reachable_wire },
			});
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
			return {};
		}

		// wpn[cn].children.emplace_back(static_cast<uint32_t>(wpn.size()));

		return std::make_unique<wire_pip_node>(wire_pip_node{
			.all_reachable_wires{std::move(n_reachable)},
			.head_reachable_wires{std::move(n_head_reachable)},
			.parent{ head_index },
			.wire_id{ head_reachable_wire },
		});
	}

	static std::vector<std::unique_ptr<wire_pip_node>> grow_head(
		const size_t head_index,
		const wire_pip_node& c_wpn,
		std::span<wire_pip_node_small> s_wpns,
		// std::vector<uint64_t> &ends,
		const PipsByWire& pipsByWire,
		const std::bitset<max_tile_wire_count>& inbound_only_wires,
		const std::bitset<max_tile_wire_count>& outbound_only_wires,
		const std::bitset<max_tile_wire_count>& bidirectional_wires,
		std::bitset<max_tile_wire_count> &reachable_wires//,
		//std::map<uint16_t, std::unordered_set<std::bitset<max_tile_wire_count>>> &map_all_paths
	) noexcept {

		std::vector<std::unique_ptr<wire_pip_node>> ret;

		if (inbound_only_wires.test(c_wpn.wire_id)) {
			return ret;
		}
		// std::cout << std::format("{} ", c_wpn.wire_id);
		auto touched_wires(get_touched_wires(head_index, s_wpns));
#if 0
		decltype(auto) found_paths{ map_all_paths[c_wpn.wire_id] };
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
		found_paths.insert(touched_wires);
#endif

		// all_paths.insert(touched_wires);

		if (c_wpn.head_reachable_wires.none()) abort();

		for (uint16_t head_reachable_wire{}; head_reachable_wire < c_wpn.head_reachable_wires.size(); head_reachable_wire++) {
			auto reachable_wire{ get_reachable_wire(
				head_index,
				head_reachable_wire,
				c_wpn,
				inbound_only_wires,
				outbound_only_wires,
				bidirectional_wires,
				touched_wires,
				pipsByWire,
				//ends,
				reachable_wires
			) };
			if (!reachable_wire) continue;
			ret.emplace_back(std::move(reachable_wire));
		}
		return ret;
	}

	static std::vector<std::unique_ptr<wire_pip_node>> grow_head_depth(
		const uint32_t base_offset,
		std::span<std::unique_ptr<wire_pip_node>> s_wpn,
		std::span<wire_pip_node_small> s_wpns,
		// std::vector<uint64_t>& ends,
		const PipsByWire& pipsByWire,
		const std::bitset<max_tile_wire_count>& inbound_only_wires,
		const std::bitset<max_tile_wire_count>& outbound_only_wires,
		const std::bitset<max_tile_wire_count>& bidirectional_wires,
		std::bitset<max_tile_wire_count>& reachable_wires
	) noexcept {

		const uint32_t num_threads{ s_wpn.size() > 128 ? std::thread::hardware_concurrency(): 1 };
		std::vector<std::vector<std::unique_ptr<wire_pip_node>>> next_wpn(num_threads);
		std::span<std::vector<std::unique_ptr<wire_pip_node>>> s_next_wpn{ next_wpn };
		{
			std::vector<std::jthread> threads;
			threads.reserve(num_threads);
			for (uint32_t thread_index = 0; thread_index < num_threads; thread_index++) {
				const uint32_t current_thread_index{ thread_index };
				threads.emplace_back([current_thread_index, num_threads, &base_offset, &s_wpn, &s_wpns, /*&ends, */&pipsByWire, &inbound_only_wires, &outbound_only_wires, &bidirectional_wires, &reachable_wires, &s_next_wpn]() {
					for (uint32_t head_index{ current_thread_index }; head_index < s_wpn.size(); head_index += num_threads) {
						auto new_heads{ grow_head(
							base_offset + head_index,
							*s_wpn[head_index].get(),
							s_wpns,
							// ends,
							pipsByWire,
							inbound_only_wires,
							outbound_only_wires,
							bidirectional_wires,
							reachable_wires
							/*map_all_paths*/
						) };
						for (auto&& new_head : new_heads) {
							s_next_wpn[current_thread_index].emplace_back(std::move(new_head));
						}
					}
				});
			}
		}
		std::vector<std::unique_ptr<wire_pip_node>> ret;
		ret.reserve(std::accumulate(s_next_wpn.begin(), s_next_wpn.end(), 0ull, [](size_t counter, std::span<std::unique_ptr<wire_pip_node>> s) { return counter + s.size(); }));
		for (auto&& next_wpn_n : next_wpn) {
			for (auto&& new_head : next_wpn_n) {
				ret.emplace_back(std::move(new_head));
			}
		}
		return ret;
	}

	class PathContainer {
	public:
		uint32_t hits{};
		uint32_t misses{};

		inline static constexpr auto is_equal{ [](const std::vector<uint16_t>& a, const std::vector<uint16_t>& b)->bool {
			return std::ranges::equal(a, b);
		} };
		inline static constexpr uint32_t hash_init{ 0x12345678ul };
		inline static constexpr auto local_hash{ [](const std::vector<uint16_t>& a)-> uint32_t {
			return std::accumulate(a.begin(), a.end(), hash_init, [](uint32_t acc, uint16_t v)-> uint32_t { return _mm_crc32_u16(acc, v); });
		} };
		std::unordered_map<std::vector<uint16_t>, uint32_t, decltype(local_hash), decltype(is_equal)> paths;
		std::map<uint64_t, std::vector<uint32_t>> wire_pairs;
		uint32_t value_count{};

		std::string show_info() const noexcept {
			return std::format("value_count: {:0.3f}M, paths: {:0.3f}M, pairs: {:0.3f}M\n",
				static_cast<double>(value_count) * 0.000001,
				static_cast<double>(paths.size()) * 0.000001,
				static_cast<double>(wire_pairs.size()) * 0.000001
			);
		}
		void write_flat_span_file(std::string base_name, std::span<uint32_t> s_u) {
			MemoryMappedFile mmf_flat_tree{ std::format("{}_u32_index.bin", base_name), s_u.size_bytes() };
			std::ranges::copy(s_u, mmf_flat_tree.get_span<uint32_t>().begin());
		}
		void write_flat_span_file(std::string base_name, std::span<uint16_t> s_u) {
			MemoryMappedFile mmf_flat_tree{ std::format("{}_u16_index.bin", base_name), s_u.size_bytes() };
			std::ranges::copy(s_u, mmf_flat_tree.get_span<uint16_t>().begin());
		}
		void write_flat_span_file(std::string base_name, std::span<flat_span> s_fs) {
			{
				MemoryMappedFile mmf_flat_tree{ std::format("{}_u32_index.bin", base_name), s_fs.size() * sizeof(uint32_t) };
				std::ranges::transform(s_fs, mmf_flat_tree.get_span<uint32_t>().begin(), [](flat_span& fs)-> uint32_t { return fs.index; });
			}
			{
				MemoryMappedFile mmf_flat_tree{ std::format("{}_u16_count.bin", base_name), s_fs.size() * sizeof(uint16_t) };
				std::ranges::transform(s_fs, mmf_flat_tree.get_span<uint16_t>().begin(), [](flat_span& fs)-> uint16_t { return static_cast<uint16_t>(fs.count); });
			}
		}

		static void skippable(std::span<flat_span> sfs) noexcept {
			std::for_each(sfs.rbegin(), sfs.rend(), [skip = 0](flat_span &fs0) mutable {
				if (fs0.count) {
					skip = 0;
				}
				else {
					if (fs0.index != UINT32_MAX) {
						abort();
					}
					--skip;
					fs0.index = skip;
				}
			});
		}

		void store_in_file() {
			std::vector<std::span<const uint16_t>> v_paths(paths.size());
			for (const auto& kv : paths) {
				v_paths[kv.second] = kv.first;
			}
			std::vector<std::vector<std::vector<std::vector<std::vector<uint32_t>>>>> tree(xcvu3p::tileType_count);
			for (const auto& kv : wire_pairs) {
				auto path_ids{ kv.second };

				const auto kp{ std::bit_cast<std::array<uint16_t, 4>>(kv.first) };
				const auto tile_type_index{ kp[3] };
				if (tile_type_index >= tree.size()) abort();
				decltype(auto) tree_tile{ tree.at(tile_type_index) };

				const auto startWire{ kp[2] };
				if (startWire >= tree_tile.size()) tree_tile.resize(startWire + 1ull);
				decltype(auto) tree_tile_startWire{ tree_tile.at(startWire) };

				const auto finishWire{ kp[1] };
				if (finishWire >= tree_tile_startWire.size()) tree_tile_startWire.resize(finishWire + 1ull);
				decltype(auto) tree_tile_startWire_finishWire{ tree_tile_startWire.at(finishWire) };

				const auto depth{ kp[0] };
				if (depth >= tree_tile_startWire_finishWire.size()) tree_tile_startWire_finishWire.resize(depth + 1ull);
				// decltype(auto) tree_tile_startWire_finishWire_depth{ tree_tile_startWire_finishWire.at(depth) };
				tree_tile_startWire_finishWire.emplace(tree_tile_startWire_finishWire.begin() + depth, std::move(path_ids));

				//for (const auto path_id : path_ids) {
				//	auto s_path{ v_paths.at(path_id) };
				//}
			}
			
			uint32_t count_flat_tree{};
			uint32_t count_flat_tile{};
			uint32_t count_flat_start{};
			uint32_t count_flat_finish{};
			uint32_t count_flat_depth{};

			std::vector<flat_span> flat_tree;
			std::vector<flat_span> flat_tile;
			std::vector<flat_span> flat_start;
			std::vector<flat_span> flat_finish;
			std::vector<uint32_t> flat_depth;
			std::vector<flat_span> flat_path;
			std::vector<uint16_t> flat_wires;

			flat_path.reserve(v_paths.size());
			for (auto&& s_wires : v_paths) {
				flat_path.emplace_back(flat_span{
					.count{static_cast<uint16_t>(s_wires.size()) },
					.index{static_cast<int32_t>(flat_wires.size())}
				});
				std::ranges::copy(s_wires, std::back_inserter(flat_wires));
			}

			for (const auto &tree_tile: tree) {
				flat_tree.emplace_back(flat_span{
					.count{static_cast<uint16_t>(tree_tile.size())},
					.index{tree_tile.empty() ? (-1) : static_cast<int32_t>(flat_tile.size()) },
				});

				for (const auto & tree_tile_startWire: tree_tile) {
					flat_tile.emplace_back(flat_span{
						.count{static_cast<uint16_t>(tree_tile_startWire.size())},
						.index{tree_tile_startWire.empty()? (-1) : static_cast<int32_t>(flat_start.size()) },
					});

					for (const auto & tree_tile_startWire_finishWire: tree_tile_startWire) {
						flat_start.emplace_back(flat_span{
							.count{static_cast<uint16_t>(tree_tile_startWire_finishWire.size())},
							.index{tree_tile_startWire_finishWire.empty()? (-1) :static_cast<int32_t>(flat_finish.size()) },
						});

						for (const auto & tree_tile_startWire_finishWire_depth: tree_tile_startWire_finishWire) {
							flat_finish.emplace_back(flat_span{
								.count{static_cast<uint16_t>(tree_tile_startWire_finishWire_depth.size())},
								.index{tree_tile_startWire_finishWire_depth.empty()? (-1) :static_cast<int32_t>(flat_depth.size())},
							});

							std::ranges::copy(tree_tile_startWire_finishWire_depth, std::back_inserter(flat_depth));
						}
					}
				}
			}

			std::span<flat_span> s_flat_tree{ flat_tree };
			std::span<flat_span> s_flat_tile{ flat_tile };
			std::span<flat_span> s_flat_start{ flat_start };
			std::span<flat_span> s_flat_finish{ flat_finish };
			std::span<uint32_t> s_flat_depth{ flat_depth };
			std::span<flat_span> s_flat_path{ flat_path };
			std::span<uint16_t> s_flat_wires{ flat_wires };

			skippable(s_flat_tree);
			skippable(s_flat_tile);
			skippable(s_flat_start);
			skippable(s_flat_finish);

			for (const auto& kv : wire_pairs) {
				auto path_ids{ kv.second };
				const auto kp{ std::bit_cast<std::array<uint16_t, 4>>(kv.first) };
				const auto tile_type_index{ kp[3] };
				const auto startWire{ kp[2] };
				const auto finishWire{ kp[1] };
				const auto depth{ kp[0] };

				const auto result0{ s_flat_tree[tile_type_index] };
				const auto& t0{ tree[tile_type_index] };

				const auto result1{ s_flat_tile[result0.index + startWire] };
				const auto& t1{ t0[startWire] };

				const auto result2{ s_flat_start[result1.index + finishWire] };
				const auto& t2{ t1[finishWire] };

				const auto result3{ s_flat_finish[result2.index + depth] };
				const auto& t3{ t2[depth] };

				const auto result4{ s_flat_depth.subspan(result3.index, result3.count) };

				if (!std::ranges::equal(t3, result4)) {
					abort();
				}

				if (!std::ranges::equal(path_ids, result4)) {
					abort();
				}
			}

			std::cout << std::format("flat_tree: {}\n", count_flat_tree);
			std::cout << std::format("flat_tile: {}\n", count_flat_tile);
			std::cout << std::format("flat_start: {}\n", count_flat_start);
			std::cout << std::format("flat_finish: {}\n", count_flat_finish);
			std::cout << std::format("flat_depth: {}\n", count_flat_depth);
			std::cout << std::format("flat_path: {}\n", flat_path.size());
			std::cout << std::format("flat_wires: {}\n", flat_wires.size());

			write_flat_span_file("flat_tree", flat_tree);
			write_flat_span_file("flat_tile", flat_tile);
			write_flat_span_file("flat_start", flat_start);
			write_flat_span_file("flat_finish", flat_finish);
			write_flat_span_file("flat_depth", flat_depth);
			write_flat_span_file("flat_path", flat_path);
			write_flat_span_file("flat_wires", flat_wires);

#if 0
			{
				MemoryMappedFile mmf_flat_path_ids{ "flat_path_ids.bin", s_flat_path_ids.size_bytes() };
				std::ranges::copy(s_flat_path_ids, mmf_flat_path_ids.get_span<flat_span>().begin());
			}
#endif

			if(true) {
				FlatPipPaths tfpp;
				std::cout << std::format("tree:{}, tile:{}, start:{}, finish:{}, depth:{}, path:{}, wires:{}\n",
					tfpp.s_flat_tree.size(),
					tfpp.s_flat_tile.size(),
					tfpp.s_flat_start.size(),
					tfpp.s_flat_finish.size(),
					tfpp.s_flat_depth_u32_index.size(),
					tfpp.s_flat_path.size(),
					tfpp.s_flat_wires_u16_index.size()
				);
				for (const auto& kv : wire_pairs) {
					auto path_ids{ kv.second };

					if (tfpp.s_flat_tree.size() != tree.size()) {
						abort();
					}
					for (uint16_t i = 0; i < tfpp.s_flat_tree.size(); i++) {
						if (tfpp.s_flat_tree.s_u16_count[i] != tree[i].size()) {
							abort();
						}
					}

					const auto kp{ std::bit_cast<std::array<uint16_t, 4>>(kv.first) };
					const auto tile_type_index{ kp[3] };
					const auto startWire{ kp[2] };
					const auto finishWire{ kp[1] };
					const auto depth{ kp[0] };

					auto sub_path_ids{ tfpp.subscript(tfpp.subscript(tile_type_index, startWire, finishWire, depth)) };

					if (!std::ranges::equal(path_ids, sub_path_ids)) {
						abort();
					}

#if 0

					auto index0{ tfpp.s_flat_tree.s_u32_index[tile_type_index] };
					auto count0{ tfpp.s_flat_tree.s_u16_count[tile_type_index] };
					SpanIndexCount result0{
						.s_u32_index{tfpp.s_flat_tile.s_u32_index.subspan(index0, count0)},
						.s_u16_count{tfpp.s_flat_tile.s_u16_count.subspan(index0, count0)}
					};//{ tfpp.flat_tree.subscript(tfpp.flat_tile, tile_type_index) };
					decltype(auto) t0{tree[tile_type_index]};

					if (result0.size() != t0.size()) {
						abort();
					}
					for (uint16_t i = 0; i < result0.size(); i++) {
						if (result0.s_u16_count[i] != t0[i].size()) {
							abort();
						}
					}
					
					auto index1{ result0.s_u32_index[startWire] };
					auto count1{ result0.s_u16_count[startWire] };
					auto result1{ result0.subscript(tfpp.s_flat_tile, startWire) };
					decltype(auto) t1{ t0[startWire] };

					if (result1.size() != t1.size()) {
						abort();
					}
					for (uint16_t i = 0; i < result1.size(); i++) {
						if (result1.s_u16_count[i] != t1[i].size()) {
							abort();
						}
					}

					auto index2{ result1.s_u32_index[finishWire] };
					auto count2{ result1.s_u16_count[finishWire] };
					auto result2{ result1.subscript(tfpp.s_flat_tile, finishWire) };
					decltype(auto) t2{ t1[finishWire] };

					if (result2.size() != t2.size()) {
						abort();
					}
					for (uint16_t i = 0; i < result2.size(); i++) {
						if (result2.s_u16_count[i] != t2[i].size()) {
							abort();
						}
					}

					auto index3{ result2.s_u32_index[depth] };
					auto count3{ result2.s_u16_count[depth] };
					auto result3{ result2.subscript(tfpp.s_flat_tile, depth) };
					decltype(auto) t3{ t2[depth] };

					if (result3.size() != t3.size()) {
						abort();
					}
					for (uint16_t i = 0; i < result3.size(); i++) {
						if (result3.s_u16_count[i] != v_paths[t3[i]].size()) {
							abort();
						}
					}



					for (uint16_t paths_index = 0; paths_index < path_ids.size(); paths_index++) {
						std::cout << std::format("tile_type_index:{}, startWire:{}, finishWire:{}, depth:{}, paths_index:{} ",
							tile_type_index, startWire, finishWire, depth, paths_index
						);
						auto found_path{ tfpp.subscript(tile_type_index, startWire, finishWire, depth, paths_index) };
						if (!std::ranges::equal(path_ids, found_path)) {
							std::cout << "not equal\n";
							abort();
						}
						std::cout << "\n";
					}
#endif
				}
			}
		}

		void set_path(const uint16_t tile_type_index, const uint16_t startWire, const uint16_t finishWire, const uint16_t depth, std::vector<uint16_t> &&wire_path) {
			auto result{ paths.insert({wire_path, static_cast<uint32_t>(paths.size())})};

			hits += (!result.second);
			misses += result.second;

			uint64_t key{ std::bit_cast<uint64_t>(std::array<uint16_t, 4>{depth, finishWire, startWire, tile_type_index }) };
			wire_pairs[key].emplace_back(result.first->second);
			value_count++;

// #ifdef _DEBUG
			if (!(value_count % 10000u)) std::cout << show_info();
// #endif

#if 0
			// if (!result.second) return;

			std::cout << std::format("hits:{}, misses:{}, startWire:{}, finishWire:{}, depth:{}, count:{}, wire_path_size:{},",
				hits,
				misses,
				startWire,
				finishWire,
				depth,
				wire_set.count(key),
				wire_path.size());
			for (auto wire_n : wire_path) {
				std::cout << std::format(" {}", wire_n);
			}
			std::cout << "\n";
#endif

		}
	};

	static void build_tile_type_wire_pip_path(
		PathContainer &pc,
		const uint16_t tile_type_idx,
		const uint16_t start_wire_index,
		const uint32_t tile_type_wire_name,
		uint32_list_reader tile_type_wires,
		const std::bitset<max_tile_wire_count>& outbound_only_wires,
		const std::bitset<max_tile_wire_count>& inbound_only_wires,
		const std::bitset<max_tile_wire_count>& bidirectional_wires,
		const size_t bidirectional_wire_count,
		const PipsByWire &pipsByWire//,
		//std::atomic<uint64_t> &total_ends,
		// std::span<intra_tile_path> s_intra_tile_paths,
		// std::atomic<uint64_t> &intra_tile_path_offset,
		// std::atomic<uint64_t>& max_v_size
	) noexcept {

		const uint16_t max_depth = 4000; // UINT16_MAX;

		if (!outbound_only_wires.test(start_wire_index)) return;
		auto initial_wpn{ make_init_wire_pip_node(start_wire_index, pipsByWire) };
		std::vector<wire_pip_node_small> wpns(1, wire_pip_node_small{ .parent{initial_wpn->parent}, .wire_id{ initial_wpn->wire_id} });
		std::bitset<max_tile_wire_count> reachable_wires;
		{
			uint32_t base_offset{};
			std::vector<std::unique_ptr<wire_pip_node>> wpn;
			wpn.emplace_back(std::move(initial_wpn));

			for (uint16_t depth_i = 0; (!wpn.empty()) && (depth_i < max_depth); ++depth_i) {
				// std::cout << std::format("{} ", wpn.size());
				// std::map<uint16_t, std::unordered_set<std::bitset<max_tile_wire_count>>> map_all_paths;
				wpn = grow_head_depth(
					base_offset,
					wpn,
					wpns,
					// ends,
					pipsByWire,
					inbound_only_wires,
					outbound_only_wires,
					bidirectional_wires,
					reachable_wires
				);
				base_offset = wpns.size();
				for (auto&& new_head : wpn) {
					wpns.emplace_back(wire_pip_node_small{ .parent{new_head->parent}, .wire_id{ new_head->wire_id} });
					if (inbound_only_wires.test(new_head->wire_id)) {
						std::vector<uint16_t> wire_path;
						wire_path.reserve(depth_i + 1ull);

						for (auto pt{ wpns.size() - 1ull }; pt != 0; pt = wpns[pt].parent) {
							if(new_head->wire_id != wpns[pt].wire_id) wire_path.emplace_back(wpns[pt].wire_id);
						}
						pc.set_path(tile_type_idx, start_wire_index, new_head->wire_id, depth_i, std::move(wire_path));
					}
				}
			}
		}
#if 0
		std::vector<uint64_t> ends;
		for (auto &&wpns_n: wpns) {
			if (inbound_only_wires.test(wpns_n.wire_id)) {
				ends.emplace_back(std::distance(wpns.data(), &wpns_n));
			}
		}
		auto my_total_ends{ total_ends.fetch_add(ends.size()) };
		std::cout << std::format("total_ends:{:6.2f}M {} bidi:{} wpn:{} ends:{} reachable:{}\n",
			std::scalbln(static_cast<double>(my_total_ends), -20), strs[tile_type_wires[start_wire_index]].cStr(),
			bidirectional_wire_count,
			wpns.size(), ends.size(), reachable_wires.count()
		);

		for (uint64_t end_idx : ends) {
			std::array<uint16_t, 31> wire_path;
			wire_path.fill(UINT16_MAX);
			size_t wire_path_offset{};
			for (auto pt{ end_idx }; pt != 0/*UINT64_MAX*/; pt = wpns[pt].parent) {
				if (wire_path_offset >= wire_path.size()) {
					abort();
				}
				wire_path[wire_path_offset] = wpns[pt].wire_id;
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
			// s_intra_tile_paths[intra_tile_path_offset++] = a_path;
			//std::for_each(wire_path.rbegin(), wire_path.rend(), [](uint16_t wire_idx) {
			//	std::cout << std::format("{} ", wire_idx);
			//});
			//std::cout << "\n";
		}
#endif
		// total_ends += wpn_ends.size();
//				std::cout << "\n\n";
		// if (wpns.size() > max_v_size) {
			//max_v_size = wpns.size();
			//std::cout << std::format("wpn.size: {}\n", wpns.size());
		//}
	}

	static void enum_tile_paths(
		PathContainer &pc,
		uint16_t tile_type_idx,
		tile_type_reader tile_type //,
		// std::atomic<uint64_t> &intra_tile_path_offset,
		 // std::atomic<uint64_t> &total_ends,
		// std::span<intra_tile_path> s_intra_tile_paths,
		//std::atomic<uint64_t> &max_v_size
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

#if 0
		std::cout << std::format("{}x {} in: {} out: {} bidi: {}\n",
			count_tile_of_type(tile_type_idx),
			tile_type_name,
			inbound_only_wire_count,
			outbound_only_wire_count,
			bidirectional_wire_count
		);
#endif


#if 0
		struct wire_pip_end {
			uint64_t wire_id;
			uint64_t parent;
		};
#endif

		// const auto intra_tile_path_offset_start{ intra_tile_path_offset.load() };

		for (uint16_t start_wire_index = 0; start_wire_index < tile_type_wires.size(); start_wire_index++) {
			build_tile_type_wire_pip_path(pc, tile_type_idx, start_wire_index, tile_type_wires[start_wire_index],
				tile_type_wires,
				outbound_only_wires,
				inbound_only_wires,
				bidirectional_wires,
				bidirectional_wire_count,
				*pipsByWire//,
				// total_ends,
				// s_intra_tile_paths,
				// intra_tile_path_offset,
				//max_v_size
			);
		}

		// const auto intra_tile_path_offset_count{ intra_tile_path_offset.load() - intra_tile_path_offset_start };
		// std::span<intra_tile_path> tile_paths{ s_intra_tile_paths.subspan(intra_tile_path_offset_start, intra_tile_path_offset_count) };
		// std::cout << std::format("sorting {} paths... ", tile_paths.size());
		// std::sort(std::execution::par_unseq, tile_paths.begin(), tile_paths.end());
		// std::cout << "done\n";

		// std::cout << std::format("\n\n");
	}

	static auto make_pip_paths() {
		if(true) {
			MemoryMappedFile mmf_intra_tile_paths{ "flat_tree_u16_count.bin" };
			if (mmf_intra_tile_paths.fsize > 0) return true;
		}
		// std::atomic<uint64_t> total_ends;
		// std::atomic<uint64_t> max_v_size{ 800000 };

		// std::vector<intra_tile_path> v_intra_tile_paths(max_intra_tile_path_count);
		// std::atomic<uint64_t> intra_tile_path_offset;

		PathContainer pc;
		for (uint16_t tile_type_idx = 0; tile_type_idx < tileTypes.size(); ++tile_type_idx) {
			enum_tile_paths(
				pc,
				tile_type_idx,
				tileTypes[tile_type_idx] //,
				// intra_tile_path_offset,
				// total_ends,
				//v_intra_tile_paths,
				//max_v_size
			);
		}

		std::cout << pc.show_info();
		pc.store_in_file();

		//auto s_intra_tile_paths{ std::span(v_intra_tile_paths).first(intra_tile_path_offset.load()) };
		//std::cout << "writing file... ";
		//{
		//	MemoryMappedFile mmf_intra_tile_paths{ "intra_tile_paths.bin", s_intra_tile_paths.size_bytes() };
		//	auto s_mmf_intra_tile_paths{ mmf_intra_tile_paths.get_span<intra_tile_path>() };
		//	std::ranges::copy(s_intra_tile_paths, s_mmf_intra_tile_paths.begin());
		//}
#if 0
		for (auto& intra_tile_path : std::span(v_intra_tile_paths).first(intra_tile_path_offset.load())) {
			std::cout << std::format("front:{} back:{} size:{}\n", intra_tile_path[0], intra_tile_path[1], intra_tile_path[2]);
		}
#endif
		return true;
	}
	inline static const decltype(make_pip_paths()) a_pip_paths{ make_pip_paths() };
	inline static const FlatPipPaths fpp;

	class NodeTileWire {
	public:
		const MemoryMappedFile a_node_counts{ MemoryMappedFile{"node_counts_u16.bin"} };
		const MemoryMappedFile a_node_offsets{ MemoryMappedFile{"node_offsets_i32.bin"} };

		const MemoryMappedFile a_node_tileIdx{ MemoryMappedFile{"node_tile_idx_u32.bin"} };
		const MemoryMappedFile a_node_tile_wire{ MemoryMappedFile{"node_tile_wire_idx_u16.bin"} };
		const MemoryMappedFile a_node_tilePos{ MemoryMappedFile{"node_tile_pos_2x_u16.bin"} };

		const MemoryMappedFile a_flat_tile_wire_to_node{ MemoryMappedFile{"flat_tile_wire_to_node_u32.bin" } };
		const MemoryMappedFile a_flat_tile_wire_offset{ MemoryMappedFile{"flat_tile_wire_offset_u32.bin" } };
		const MemoryMappedFile a_flat_tile_wire_count{ MemoryMappedFile{"flat_tile_wire_count_u16.bin" } };

		const std::span<const uint16_t, xcvu3p::node_count> s_node_counts{ a_node_counts.get_span<const uint16_t, xcvu3p::node_count>() };
		const std::span<const int32_t, xcvu3p::node_count> s_node_offsets{ a_node_offsets.get_span<const int32_t, xcvu3p::node_count>() };

		const std::span<const uint32_t, xcvu3p::wire_count> s_node_tileIdx{ a_node_tileIdx.get_span<const uint32_t, xcvu3p::wire_count>() };
		const std::span<const uint16_t, xcvu3p::wire_count> s_node_tile_wire{ a_node_tile_wire.get_span<const uint16_t, xcvu3p::wire_count>() };
		const std::span<const std::array<uint16_t, 2>, xcvu3p::wire_count> s_node_tilePos{ a_node_tilePos.get_span<const std::array<uint16_t, 2>, xcvu3p::wire_count>() };

		const std::span<const uint32_t, xcvu3p::wire_count> s_flat_tile_wire_to_node{ a_flat_tile_wire_to_node.get_span<const uint32_t, xcvu3p::wire_count>() };
		const std::span<const uint32_t, xcvu3p::tile_count> s_flat_tile_wire_offset{ a_flat_tile_wire_offset.get_span<const uint32_t, xcvu3p::tile_count>() };
		const std::span<const uint16_t, xcvu3p::tile_count> s_flat_tile_wire_count{ a_flat_tile_wire_count.get_span<const uint16_t, xcvu3p::tile_count>() };

		inline auto get_tile_wire_nodeIdx(uint32_t tileIdx, uint16_t tile_wire_idx) const noexcept {
			std::span<const uint32_t> tile_nodes{ s_flat_tile_wire_to_node.subspan(s_flat_tile_wire_offset[tileIdx], s_flat_tile_wire_count[tileIdx])};
			return tile_nodes[tile_wire_idx];
		}
	};

	bool enum_node_wire_tile_tw() {
		{
			const MemoryMappedFile a_node_counts{ MemoryMappedFile{"node_counts_u16.bin"} };
			if (a_node_counts.fsize == (sizeof(uint16_t) * xcvu3p::node_count)) {
				return true;
			}
		}
		auto a_node_counts{ MemoryMappedFile{"node_counts_u16.bin", sizeof(uint16_t) * xcvu3p::node_count} };
		auto a_node_offsets{ MemoryMappedFile{"node_offsets_i32.bin", sizeof(int32_t) * xcvu3p::node_count} };

		auto a_node_tileIdx{ MemoryMappedFile{"node_tile_idx_u32.bin", sizeof(uint32_t) * xcvu3p::wire_count} };
		auto a_node_tile_wire{ MemoryMappedFile{"node_tile_wire_idx_u16.bin", sizeof(uint16_t) * xcvu3p::wire_count} };
		auto a_node_tilePos{ MemoryMappedFile{"node_tile_pos_2x_u16.bin", sizeof(std::array<uint16_t, 2>) * xcvu3p::wire_count} };
		auto a_tileStrIdx_to_tileIdx{ std::make_unique<std::array<uint32_t, xcvu3p::str_count>>() };
		auto a_tileTypeWires{ std::make_unique<std::array<std::array<uint32_t, xcvu3p::str_count>, xcvu3p::tileType_count>>() };

		auto a_flat_tile_wire_to_node{ MemoryMappedFile{"flat_tile_wire_to_node_u32.bin", xcvu3p::wire_count * sizeof(uint32_t) } };
		auto a_flat_tile_wire_offset{ MemoryMappedFile{"flat_tile_wire_offset_u32.bin", xcvu3p::tile_count * sizeof(uint32_t) } };
		auto a_flat_tile_wire_count{ MemoryMappedFile{"flat_tile_wire_count_u16.bin", xcvu3p::tile_count * sizeof(uint16_t) } };

		auto s_flat_tile_wire_to_node{ a_flat_tile_wire_to_node.get_span<uint32_t, xcvu3p::wire_count>() };
		auto s_flat_tile_wire_offset{ a_flat_tile_wire_offset.get_span<uint32_t, xcvu3p::tile_count>() };
		auto s_flat_tile_wire_count{ a_flat_tile_wire_count.get_span<uint16_t, xcvu3p::tile_count>() };

		uint32_t tile_wire_to_node_offset{};

		auto a_tile_wire_to_node{ std::make_unique<std::array<std::span<uint32_t>, xcvu3p::tile_count>>() };
		auto s_tile_wire_to_node{ std::span(*a_tile_wire_to_node.get()) };

		for (uint32_t tileIdx = 0; tileIdx < xcvu3p::tile_count; tileIdx++) {
			const auto tile{ xcvu3p::tiles[tileIdx] };
			const auto tileTypeIdx{ tile.getType() };
			const auto tileType{ xcvu3p::tileTypes[tileTypeIdx] };
			const auto tileTypeWireSize{ tileType.getWires().size() };

			s_flat_tile_wire_offset[tileIdx] = tile_wire_to_node_offset;
			s_flat_tile_wire_count[tileIdx] = tileTypeWireSize;
			s_tile_wire_to_node[tileIdx] = s_flat_tile_wire_to_node.subspan(tile_wire_to_node_offset, tileTypeWireSize);
			tile_wire_to_node_offset += tileTypeWireSize;
		}

		auto s_node_tileIdx{ a_node_tileIdx.get_span<uint32_t, xcvu3p::wire_count>() };
		auto s_node_tile_wire{ a_node_tile_wire.get_span<uint16_t, xcvu3p::wire_count>() };
		auto s_node_tilePos{ a_node_tilePos.get_span<std::array<uint16_t, 2>, xcvu3p::wire_count>() };
		auto s_node_counts{ a_node_counts.get_span<uint16_t, xcvu3p::node_count>() };
		auto s_node_offsets{ a_node_offsets.get_span<int32_t, xcvu3p::node_count>() };
		auto s_tileStrIdx_to_tileIdx{ std::span(*a_tileStrIdx_to_tileIdx.get()) };
		std::vector<std::span<uint32_t, xcvu3p::str_count>> vs_tileTypeWires;
		vs_tileTypeWires.reserve(xcvu3p::tileType_count);
		std::ranges::copy(*a_tileTypeWires.get(), std::back_inserter(vs_tileTypeWires));
		auto ss_tileTypeWires{ std::span(vs_tileTypeWires) };

		std::ranges::fill(s_tileStrIdx_to_tileIdx, UINT32_MAX);
		std::ranges::for_each(vs_tileTypeWires, [](std::span<uint32_t, xcvu3p::str_count> ttwires) { std::ranges::fill(ttwires, UINT32_MAX); });

		for (uint32_t tileIdx = 0; tileIdx < xcvu3p::tile_count; tileIdx++) {
			auto tile{ xcvu3p::tiles[tileIdx] };
			auto tileTypeIdx{ tile.getType() };
			auto tileType{ xcvu3p::tileTypes[tileTypeIdx] };

			s_tileStrIdx_to_tileIdx[xcvu3p::tiles[tileIdx].getName()] = tileIdx;
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
				s_node_tile_wire[node_offset + nodeWireIdx] = tileWireIdx;
				s_node_tilePos[node_offset + nodeWireIdx] = std::array<uint16_t, 2>{tile.getCol(), tile.getRow()};
				});
			});
	}

	inline static bool results_enum_node_wire_tile_tw{ enum_node_wire_tile_tw() };
	NodeTileWire ntw;
};

class Tile_Based_Router {
public:

	static inline constexpr size_t bans_per_tile{ 100ull };
	static inline constexpr size_t shader_workgroup_size{ 256ull };
	static inline constexpr size_t tile_shader_count{ ((xcvu3p::tile_count + (shader_workgroup_size - 1ull)) / shader_workgroup_size) * shader_workgroup_size };
	static inline constexpr size_t total_ban_count{ bans_per_tile * tile_shader_count };

	phys_reader& phys;
	net_list_reader nets;
	std::vector<uint32_t> net_source_tileIdx;
	std::vector<uint16_t> net_source_tileWire;
	std::vector<std::array<int16_t, 2>> net_source_tilePos;

//host side
	const std::vector<uint32_t> v_physStrs_to_devStrs;
	const std::span<const uint32_t> s_physStrs_to_devStrs;
	std::unique_ptr<std::array<uint32_t, xcvu3p::wire_count>> a_tile_wire_net_id;
	std::unique_ptr<std::array<uint32_t, xcvu3p::wire_count>> a_tile_wire_path_id;
	std::unique_ptr<std::array<uint32_t, tile_shader_count>> a_tile_wire_offset; //readonly, one per tile
	std::unique_ptr<std::array<uint16_t, tile_shader_count>> a_tile_wire_count; //readonly, one per tile

//	std::unique_ptr<aligned_array<uint32_t, xcvu3p::str_count>> a_devStrs_to_physStrs;
//	std::span<uint32_t, xcvu3p::str_count> s_devStrs_to_physStrs;

//	std::unique_ptr<aligned_array<uint16_t, xcvu3p::wire_count>> a_inbox_modified_wires;
//	std::unique_ptr<aligned_array<uint32_t, total_ban_count>> a_ban_net_ids;
//	std::unique_ptr<aligned_array<uint16_t, total_ban_count>> a_ban_tile_wire_ids;

//shader side
	std::span<uint32_t, xcvu3p::wire_count> s_tile_wire_net_id;
	std::span<uint32_t, xcvu3p::wire_count> s_tile_wire_path_id;
	//	std::span<uint32_t, xcvu3p::wire_count> s_tile_wire_previous_tile_id;
//	std::span<uint16_t, xcvu3p::wire_count> s_tile_wire_previous_tile_wire;
//	std::span<uint16_t, xcvu3p::wire_count> s_inbox_modified_wires;

//	std::span<uint32_t, total_ban_count> s_ban_net_ids;
//	std::span<uint16_t, total_ban_count> s_ban_tile_wire_ids;

	std::span<uint32_t, tile_shader_count> s_tile_wire_offset; //readonly, one per tile
	std::span<uint16_t, tile_shader_count> s_tile_wire_count; //readonly, one per tile

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
		net_source_tileIdx{ std::vector<uint32_t>(static_cast<size_t>(nets.size()), UINT32_MAX) },
		net_source_tileWire{ std::vector<uint16_t>(static_cast<size_t>(nets.size()), UINT16_MAX) },
		net_source_tilePos{ std::vector<std::array<int16_t, 2>>(static_cast<size_t>(nets.size()), std::array<int16_t, 2>{INT16_MAX, INT16_MAX}) },
		v_physStrs_to_devStrs{ make_physStrs_to_devStrs(phys.getStrList()) },
		s_physStrs_to_devStrs{ v_physStrs_to_devStrs },
		a_tile_wire_net_id{ std::make_unique<std::array<uint32_t, xcvu3p::wire_count>>() },
		a_tile_wire_path_id{ std::make_unique<std::array<uint32_t, xcvu3p::wire_count>>() },
		a_tile_wire_offset{ std::make_unique<std::array<uint32_t, tile_shader_count>>() },
		a_tile_wire_count{ std::make_unique<std::array<uint16_t, tile_shader_count>>() },
		s_tile_wire_net_id{ *a_tile_wire_net_id.get() },
		s_tile_wire_path_id{ *a_tile_wire_path_id.get() },
		s_tile_wire_offset{ *a_tile_wire_offset.get() },
		s_tile_wire_count{ *a_tile_wire_count.get() }
	{
		std::ranges::fill(s_tile_wire_net_id, UINT32_MAX);
		std::ranges::fill(s_tile_wire_path_id, UINT32_MAX);
		std::ranges::fill(s_tile_wire_offset, UINT32_MAX);
		std::ranges::fill(s_tile_wire_count, 0);
		uint32_t tile_wire_offset{};
		each<uint32_t>(xcvu3p::tiles, [&](uint32_t tile_idx, tile_reader tile) {
			auto tile_type_wires{ xcvu3p::tileTypes[tile.getType()].getWires() };
			s_tile_wire_offset[tile_idx] = tile_wire_offset;
			s_tile_wire_count[tile_idx] = tile_type_wires.size();
			tile_wire_offset += tile_type_wires.size();
			// each<uint16_t>(tile_type_wires, [&](uint16_t tile_wire_idx, uint32_t wire_str) {
			//
			//});
		});
	}

#if 0
	auto get_shader_tile_wire(uint32_t wire_idx) noexcept-> Tile_Wire& {
		decltype(auto) tw{ xcvu3p::wire_idx_to_tile_idx_tile_wire_idx[wire_idx] };
		decltype(auto) tile_info{ s_tile_infos[tw.tile_idx] };
		auto stw{ s_tile_wires.subspan(tile_info.tile_wire_offset, tile_info.tile_wire_count) };
		return stw[tw.tile_wire_idx];
	}
#endif

	std::span<uint32_t> get_tile_net_ids(const uint32_t tile_idx) const noexcept {
		return s_tile_wire_net_id.subspan(s_tile_wire_offset[tile_idx], s_tile_wire_count[tile_idx]);
	}

	std::span<uint32_t> get_tile_path_ids(const uint32_t tile_idx) const noexcept {
		return s_tile_wire_path_id.subspan(s_tile_wire_offset[tile_idx], s_tile_wire_count[tile_idx]);
	}

	void set_shader_tile_wire(const uint32_t wire_idx, const uint32_t net_id, const uint32_t previous_tile_id = UINT32_MAX, const uint16_t previous_tile_wire = UINT16_MAX) noexcept {
		decltype(auto) tw{ xcvu3p::wire_idx_to_tile_idx_tile_wire_idx[wire_idx] };
		auto tile_net_ids{ get_tile_net_ids(tw.tile_idx) };
		tile_net_ids[tw.tile_wire_idx] = net_id;
		net_source_tileIdx[net_id] = tw.tile_idx;
		net_source_tileWire[net_id] = tw.tile_wire_idx;
		auto tile{ xcvu3p::tiles[tw.tile_idx] };
		net_source_tilePos[net_id] = { static_cast<int16_t>(tile.getCol()), static_cast<int16_t>(tile.getRow()) };
	}

	void set_shader_tile_wire_inbox(const uint32_t wire_idx, const uint32_t net_id, const uint32_t previous_tile_id = UINT32_MAX, const uint16_t previous_tile_wire = UINT16_MAX) noexcept {
		decltype(auto) tw{ xcvu3p::wire_idx_to_tile_idx_tile_wire_idx[wire_idx] };
		auto tile_net_ids{ get_tile_net_ids(tw.tile_idx) };
		auto tileTypeIdx{ xcvu3p::tiles[tw.tile_idx].getType() };
		auto finishes{ xcvu3p::fpp.subscript_by_wireIdx(wire_idx) };
		if (finishes.empty()) {
			const auto node_idx{ xcvu3p::wire_idx_to_node_idx[wire_idx] };
			const auto node{ xcvu3p::nodes[node_idx] };
			const auto node_wires{ node.getWires() };
			std::vector<uint32_t> v_usable_wire_idx;
			for (auto node_wire_idx : node_wires) {
				auto fn{ xcvu3p::fpp.subscript_by_wireIdx(node_wire_idx) };
				if (fn.empty()) continue;
				// std::cout << std::format(" wire_idx:{} node_wire_idx: {} fn: {}\n", wire_idx, node_wire_idx, fn.size());
#if 0
				for (uint16_t finish_wire_idx = 0; finish_wire_idx < fn.size();) {
					auto depths{ xcvu3p::fpp.subscript(fn, finish_wire_idx) };
					std::cout << std::format("  finish_wire_idx: {}, depths: {}:{}\n", finish_wire_idx, depths.size(), depths.index);
					if (depths.empty()) {
						finish_wire_idx -= depths.index;
						continue;
					}
					else {
						++finish_wire_idx;
					}
					for (uint16_t depth_idx = 0; depth_idx < depths.size();) {
						auto path_ids{ xcvu3p::fpp.subscript(depths, depth_idx) };
						std::cout << std::format("   depth: {}, path_ids: {}:{}\n", depth_idx, path_ids.size(), path_ids.index);
						if (path_ids.empty()) {
							depth_idx -= path_ids.index;
							continue;
						}
						else {
							// std::cout << std::format("   depth: {}, path_ids: {}\n", depth_idx, path_ids.size());
							++depth_idx;
						}
					}
				}
#endif
				v_usable_wire_idx.emplace_back(node_wire_idx);
			}
			if (v_usable_wire_idx.size()) {
				// std::cout << std::format("v_usable_wire_idx.size(): {}\n", v_usable_wire_idx.size());
				set_shader_tile_wire_inbox(v_usable_wire_idx.front(), net_id, previous_tile_id, previous_tile_wire);
				return;
			}
#if 0
			std::cout << std::format("tileTypeIdx:{} start: {} finishes: {}\n",
				xcvu3p::strs[xcvu3p::tileTypes[tileTypeIdx].getName()].cStr(),
				xcvu3p::strs[xcvu3p::tileTypes[tileTypeIdx].getWires()[tw.tile_wire_idx]].cStr(),
				finishes.size());
#endif
			return;
		}
		// std::cout << std::format("tileTypeIdx:{} start: {} finishes: {}\n", tileTypeIdx, tw.tile_wire_idx, finishes.size());
		tile_net_ids[tw.tile_wire_idx] = net_id | 0x8000'0000u;
	}

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
				set_shader_tile_wire(node_wire_idx, net_idx);
			}
		} else {
			set_shader_tile_wire_inbox(wire_idx, net_idx);
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
		each<uint32_t>(nets, [&](uint32_t net_idx, net_reader net) {
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
#if 1
		each<uint32_t>(s_tile_wire_offset, [&](const uint32_t tile_idx, const uint32_t wire_offset) -> void {
			if (wire_offset == UINT32_MAX) return;
			auto tile_net_ids{ get_tile_net_ids(tile_idx) };
			auto tile_path_ids{ get_tile_path_ids(tile_idx) };
			auto tile_type_idx{ xcvu3p::tiles[tile_idx].getType() };

			each<uint16_t>(tile_net_ids, [&](const uint16_t tile_wire_idx, uint32_t &ib_net_id) -> void {
				if (ib_net_id == UINT32_MAX) return;
				const bool isInbox{ static_cast<bool>(ib_net_id >> 31u) };
				const uint32_t net_id{ ib_net_id & 0x7fff'ffffu };
				if (!isInbox) return;
				auto available_finish_wires{ xcvu3p::fpp.subscript(tile_type_idx, tile_wire_idx) };
				if (available_finish_wires.empty()) return;
				
				const auto nodeIdx{ xcvu3p::ntw.get_tile_wire_nodeIdx(tile_idx, tile_wire_idx) };

				const auto source_tileIdx{ net_source_tileIdx[net_id] };
				const auto source_tileWire{net_source_tileWire[net_id] };
				const auto source_tilePos{ net_source_tilePos[net_id] };
				const auto cur_tilePos{ std::array<int16_t, 2>{
					static_cast<int16_t>(xcvu3p::tiles[tile_idx].getCol()),
					static_cast<int16_t>(xcvu3p::tiles[tile_idx].getRow()),
				} };
				if (source_tileIdx == UINT32_MAX) return;
				const auto delta{ std::array<int16_t, 2>{
					static_cast<int16_t>(source_tilePos[0] - cur_tilePos[0]),
					static_cast<int16_t>(source_tilePos[1] - cur_tilePos[1]),
				} };

				const auto tile_dist{ std::sqrt(
					std::pow(static_cast<double>(delta[0]), 2) +
					std::pow(static_cast<double>(delta[1]), 2)
				)};

				if (source_tileIdx == tile_idx) {
#if 1
					std::cout << std::format("tile:{} srcTile:{} tile:{},{}=> src:{:4d},{:4d} delta: {:4d},{:4d} dist: {:7.3f} type:{} start:{} node:{} node_wires:{} is:{} net:{} avail:{}\n",
						xcvu3p::strs[xcvu3p::tiles[tile_idx].getName()].cStr(),
						xcvu3p::strs[xcvu3p::tiles[source_tileIdx].getName()].cStr(),
						xcvu3p::tiles[tile_idx].getCol(), xcvu3p::tiles[tile_idx].getRow(),
						source_tilePos[0], source_tilePos[1],
						delta[0], delta[1],
						tile_dist,
						xcvu3p::strs[xcvu3p::tileTypes[tile_type_idx].getName()].cStr(),
						xcvu3p::strs[xcvu3p::tileTypes[tile_type_idx].getWires()[tile_wire_idx]].cStr(),
						xcvu3p::strs[xcvu3p::wires[xcvu3p::nodes[nodeIdx].getWires()[0]].getWire()].cStr(),
						xcvu3p::ntw.s_node_counts[nodeIdx],
						isInbox,
						phys.getStrList()[phys.getPhysNets()[net_id].getName()].cStr(),
						available_finish_wires.size()
					);
#endif

					auto depths{ xcvu3p::fpp.subscript(available_finish_wires, source_tileWire) };
					std::cout << std::format("  depths: {}\n", depths.size());
					bool pathAssigned{ false };
					for (uint16_t depth_idx = 0; (!pathAssigned) && depth_idx < depths.size();) {
						auto path_ids{ xcvu3p::fpp.subscript(depths, depth_idx) };
						if (path_ids.empty()) {
							depth_idx -= path_ids.index;
							continue;
						}
						else {
							// std::cout << std::format("   depth: {}, path_ids: {}\n", depth_idx, path_ids.size());
							std::cout << std::format("   depth: {}, path_ids: {}\n", depth_idx, path_ids.size());
							for (uint16_t path_idx = 0; (!pathAssigned) && path_idx < path_ids.size(); ++path_idx) {
								auto path_n{ xcvu3p::fpp.subscript(path_ids, path_idx) };
								// std::cout << "    ";
								bool pathAvailable{ true };
								for (auto path_step_wire : path_n) {
									// std::cout << std::format("{} ", path_step_wire);
									if (tile_net_ids[path_step_wire] != UINT32_MAX && (tile_net_ids[path_step_wire] & 0x7fff'ffffu) != net_id) {
										// std::cout << std::format("used by {}", tile_net_ids[path_step_wire]);
										pathAvailable = false;
										break;
									}
								}
								if (pathAvailable) {
									std::cout << "   routed in tile\n";
									pathAssigned = true;
									tile_path_ids[tile_wire_idx] = xcvu3p::fpp.subscript_path_id(path_ids, path_idx);
									tile_net_ids[tile_wire_idx] &= 0x7fff'ffffu;
									for (auto path_step_wire : path_n) {
										tile_net_ids[path_step_wire] = net_id;
									}

									break;
								}
								else {

								}
								// std::cout << "\n";
							}
							if (pathAssigned) {
								break;
							}
							++depth_idx;
						}
					}
					if (!pathAssigned) {
						std::cout << "   unroutable in tile\n";
					}
				}

				return;
				for (uint16_t finish_wire_idx = 0; finish_wire_idx < available_finish_wires.size();) {
					auto depths{ xcvu3p::fpp.subscript(available_finish_wires, finish_wire_idx) };
					if (depths.empty()) {
						finish_wire_idx -= depths.index;
						continue;
					}
					else {
						std::cout << std::format("  finish_wire_idx: {}, depths: {}\n", finish_wire_idx, depths.size());
						++finish_wire_idx;
					}
					for (uint16_t depth_idx = 0; depth_idx < depths.size();) {
						auto path_ids{ xcvu3p::fpp.subscript(depths, depth_idx) };
						if (path_ids.empty()) {
							depth_idx -= path_ids.index;
							continue;
						}
						else {
							// std::cout << std::format("   depth: {}, path_ids: {}\n", depth_idx, path_ids.size());
							std::cout << std::format("   depth: {}, path_ids: {}\n", depth_idx, path_ids.size());
							++depth_idx;
						}
					}
				}


				// each<uint16_t>(available_finish_wires.s_u16_count, [&](uint16_t finish_wire_idx, uint16_t depth_count) {
				//	if (!depth_count) return;
				//	std::cout << std::format("  finish:{} depths:{}\n", finish_wire_idx, depth_count);
				//});
			});
		});
#endif
		return true;
	}
};

static_assert(alignof(aligned_array<uint32_t, 1024>) == alignof(__m512i));
