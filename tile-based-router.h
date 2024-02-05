#pragma once

#include "lib_dev_tile_index.h"
#include "lib_inverse_wires.h"
#include "lib_site_pin_to_wire.h"
#include "lib_site_pin_to_node.h"
#include "lib_wire_idx_to_tile_idx_tile_wire_idx.h"

#if 1
class Tile_Based_Router {
public:

	static inline const size_t bans_per_tile{ 100ull };

	class Tile_Info {
	public:
		uint32_t tile_id;
		uint32_t tile_wire_offset;
		uint32_t inbox_offset;
		uint32_t ban_offset;

		uint16_t inbox_read_pos;
		uint16_t inbox_write_pos;
		uint16_t tile_wire_count;
		uint16_t inbox_count;
		uint16_t ban_count;
		uint16_t reseverd_area[3];

		static Tile_Info make(
			uint32_t tile_id,
			std::atomic<uint32_t> &offset_tiles,
			std::atomic<uint32_t> &offset_tile_wires,
			std::atomic<uint32_t> &offset_inbox_items,
			std::atomic<uint32_t> &offset_bans,
			const uint16_t tile_wire_count,
			const uint16_t inbox_count,
			const uint16_t ban_count
		) {
			return Tile_Info{
			//uint32_t
				.tile_id{tile_id},
				.tile_wire_offset{ offset_tile_wires.fetch_add(tile_wire_count) }, // fixme only need inbound wires
				.inbox_offset{ offset_inbox_items.fetch_add(inbox_count) },  // fixme only need inbound wires
				.ban_offset{ offset_bans.fetch_add(ban_count) }, //fixme find what works, runs out, etc
			//uint16_t
				.inbox_read_pos{},
				.inbox_write_pos{},
				.tile_wire_count{ tile_wire_count },
				.inbox_count{ inbox_count },
				.ban_count{ ban_count },
			};
		}
	};

	class Tile_Wire {
	public:
		uint32_t net_id;
		uint16_t previous_tile_col, previous_tile_row, previous_tile_wire;
	};

	class Inbox_Item {
	public:
		uint16_t modified_wire;
	};

	class Ban {
	public:
		uint32_t net_id;
		uint16_t wire_id;
	};

//host side
	std::vector<uint32_t> v_tile_id_to_shader_tile_id;
	std::span<uint32_t> s_tile_id_to_shader_tile_id;

	std::vector<uint32_t> v_physStrs_to_devStrs;
	std::span<uint32_t> s_physStrs_to_devStrs;

	std::vector<uint32_t> v_devStrs_to_physStrs;
	std::span<uint32_t> s_devStrs_to_physStrs;

//shader side
	std::vector<Tile_Info> v_tile_infos;
	std::span<Tile_Info> s_tile_infos;

	std::vector<Tile_Wire> v_tile_wires;
	std::span<Tile_Wire> s_tile_wires;

	std::vector<Inbox_Item> v_inbox_items;
	std::span<Inbox_Item> s_inbox_items;

	std::vector<Ban> v_bans;
	std::span<Ban> s_bans;

	phys_reader &phys;
	net_list_reader nets;

	Tile_Based_Router() = delete;
	Tile_Based_Router(Tile_Based_Router& other) = delete;
	Tile_Based_Router& operator=(Tile_Based_Router& other) = delete;
	Tile_Based_Router operator=(Tile_Based_Router other) = delete;
	Tile_Based_Router(Tile_Based_Router&& other) = delete;
	Tile_Based_Router& operator=(Tile_Based_Router&& other) = delete;

	Tile_Based_Router(
		//host side
		std::vector<uint32_t> &&_v_tile_id_to_shader_tile_id,
		std::vector<uint32_t> &&_v_physStrs_to_devStrs,
		std::vector<uint32_t> &&_v_devStrs_to_physStrs,
		//shader side
		std::vector<Tile_Info> &&_v_tile_infos,
		std::vector<Tile_Wire> &&_v_tile_wires,
		std::vector<Inbox_Item> &&_v_inbox_items,
		std::vector<Ban> &&_v_bans,
		phys_reader &_phys
	) noexcept :
		//host side
		v_tile_id_to_shader_tile_id{_v_tile_id_to_shader_tile_id },
		s_tile_id_to_shader_tile_id{ v_tile_id_to_shader_tile_id },

		v_physStrs_to_devStrs{ _v_physStrs_to_devStrs },
		s_physStrs_to_devStrs{ v_physStrs_to_devStrs },

		v_devStrs_to_physStrs{ _v_devStrs_to_physStrs },
		s_devStrs_to_physStrs{ v_devStrs_to_physStrs },

		//shader side
		v_tile_infos{ _v_tile_infos },
		s_tile_infos{ v_tile_infos },

		v_tile_wires{ _v_tile_wires },
		s_tile_wires{ v_tile_wires },

		v_inbox_items{ _v_inbox_items },
		s_inbox_items{ v_inbox_items },

		v_bans{ _v_bans },
		s_bans{ v_bans },

		phys{ _phys },
		nets{ phys.getPhysNets() }
	{
	}

	auto get_shader_tile_wire(uint32_t wire_idx) noexcept-> Tile_Wire& {
		decltype(auto) tw{ xcvu3p::wire_idx_to_tile_idx_tile_wire_idx[wire_idx] };
		auto shader_tile_id{ s_tile_id_to_shader_tile_id[tw.tile_idx] };
		decltype(auto) tile_info{ s_tile_infos[shader_tile_id] };
		auto stw{ s_tile_wires.subspan(tile_info.tile_wire_offset, tile_info.tile_wire_count) };
		return stw[tw.tile_wire_idx];
	}

	void set_shader_tile_wire(uint32_t wire_idx, Tile_Wire&& ntw) noexcept {
		get_shader_tile_wire(wire_idx) = ntw;
	}

	void set_shader_tile_wire_inbox(uint32_t wire_idx, Tile_Wire&& ntw) noexcept {
		decltype(auto) tw{ xcvu3p::wire_idx_to_tile_idx_tile_wire_idx[wire_idx] };
		auto shader_tile_id{ s_tile_id_to_shader_tile_id[tw.tile_idx] };
		decltype(auto) tile_info{ s_tile_infos[shader_tile_id] };
		auto stw{ s_tile_wires.subspan(tile_info.tile_wire_offset, tile_info.tile_wire_count) };
		stw[tw.tile_wire_idx] = ntw;
		auto sti{ s_inbox_items.subspan(tile_info.inbox_offset, tile_info.inbox_count) };
		auto inbox_write_pos{ tile_info.inbox_write_pos++ };
		sti[inbox_write_pos].modified_wire = tw.tile_wire_idx;
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
				set_shader_tile_wire(node_wire_idx, { .net_id{net_idx}, .previous_tile_col{UINT16_MAX}, .previous_tile_row{UINT16_MAX}, .previous_tile_wire{UINT16_MAX} });
			}
		} else {
			set_shader_tile_wire_inbox(wire_idx, { .net_id{net_idx}, .previous_tile_col{UINT16_MAX}, .previous_tile_row{UINT16_MAX}, .previous_tile_wire{UINT16_MAX} });
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
		jthread_each(nets, [&](uint64_t net_idx, net_reader net) {
			block_resources(net_idx, net);
		});

		return true;
	}

	static std::map<std::string_view, std::array<uint16_t, 2>> make_site_locations(device_reader dev) {
		auto devStrs{ dev.getStrList() };
		std::map<std::string_view, std::array<uint16_t, 2>> site_locations;
		for (auto&& tile : dev.getTileList()) {
			std::array<uint16_t, 2> pos{ tile.getCol(), tile.getRow() };
			for (auto&& site : tile.getSites()) {
				site_locations.insert({ devStrs[site.getName()].cStr(), pos });
			}
		}
		return site_locations;
	}

	static std::unordered_map<std::string_view, uint32_t> make_devStrs_map(DeviceResources::Device::Reader dev) {
		std::unordered_map<std::string_view, uint32_t> devStrs_map;
		auto devStrs{ dev.getStrList() };
		devStrs_map.reserve(devStrs.size());

		each<uint32_t, decltype(devStrs), decltype(devStrs[0])>(devStrs, [&](uint32_t dev_strIdx, capnp::Text::Reader dev_str) {
			std::string_view str{ dev_str.cStr() };
			devStrs_map.insert({ str, dev_strIdx });
			});
		return devStrs_map;
	}

	static std::array<std::vector<uint32_t>, 2> make_string_interchange(const std::unordered_map<std::string_view, uint32_t>& devStrs_map, ::capnp::List< ::capnp::Text, ::capnp::Kind::BLOB>::Reader devStrs, ::capnp::List< ::capnp::Text, ::capnp::Kind::BLOB>::Reader physStrs) {
		std::vector<uint32_t> physStrs_to_devStrs(static_cast<size_t>(physStrs.size()), UINT32_MAX);
		std::vector<uint32_t> devStrs_to_physStrs(static_cast<size_t>(devStrs.size()), UINT32_MAX);
		each<uint32_t, decltype(physStrs), decltype(physStrs[0])>(physStrs, [&](uint32_t phys_strIdx, capnp::Text::Reader phys_str) {
			std::string_view str{ phys_str.cStr() };
			if (!devStrs_map.contains(str)) return;
			auto dev_strIdx{ devStrs_map.at(str) };
			physStrs_to_devStrs[phys_strIdx] = dev_strIdx;
			devStrs_to_physStrs[dev_strIdx] = phys_strIdx;
		});
		return { physStrs_to_devStrs , devStrs_to_physStrs };
	}

	static Tile_Based_Router make(phys_reader &phys) {
		std::atomic<uint32_t> needed_tiles, needed_tile_wires, needed_inbox_items, needed_bans;
		jthread_each(xcvu3p::tiles, [&](uint64_t tile_idx, tile_reader& tile) {
			auto tile_type{ xcvu3p::tileTypes[tile.getType()] };
			auto tile_wires{ tile_type.getWires() };
			auto tile_pips{ tile_type.getPips() };
			if (!tile_wires.size()) return;
			needed_tiles++;
			needed_tile_wires += tile_wires.size();// fixme only need inbound wires
			needed_inbox_items += tile_wires.size();// fixme only need inbound wires
			needed_bans += bans_per_tile; //fixme find what works, runs out, etc
		});

		auto tile_infos{ std::vector<Tile_Info>(static_cast<size_t>(needed_tiles.load()), Tile_Info{}) };
		auto tile_wires{ std::vector<Tile_Wire>(static_cast<size_t>(needed_tile_wires.load()), Tile_Wire{}) };
		auto inbox_items{ std::vector<Inbox_Item>(static_cast<size_t>(needed_inbox_items.load()), Inbox_Item{}) };
		auto bans{ std::vector<Ban>(static_cast<size_t>(needed_bans.load()), Ban{}) };

		std::vector<uint32_t> tile_id_to_shader_tile_id(static_cast<size_t>(xcvu3p::tiles.size()), UINT32_MAX);

		std::atomic<uint32_t> offset_tiles, offset_tile_wires, offset_inbox_items, offset_bans;
		jthread_each(xcvu3p::tiles, [&](uint64_t tile_idx, tile_reader& tile) {
			auto tile_type{ xcvu3p::tileTypes[tile.getType()] };
			auto tile_wires{ tile_type.getWires() };
			auto tile_pips{ tile_type.getPips() };
			if (!tile_wires.size()) return;

			auto tile_offset{ offset_tiles.fetch_add(1) };

			Tile_Info &tile_info{ tile_infos[tile_offset] = Tile_Info::make(
				static_cast<uint32_t>(tile_idx),
				offset_tiles, offset_tile_wires, offset_inbox_items, offset_bans,
				tile_wires.size(),
				tile_wires.size(),
				bans_per_tile
			)};
			tile_id_to_shader_tile_id[tile_idx] = tile_offset;
		});

		auto site_locations{ TimerVal(make_site_locations(xcvu3p::root)) };
		const auto devStrs_map{ TimerVal(make_devStrs_map(xcvu3p::root)) };

		string_list_reader physStrs{ phys.getStrList() };
		auto string_interchange{ TimerVal(make_string_interchange(devStrs_map, xcvu3p::strs, physStrs)) };
		auto physStrs_to_devStrs{ std::move(string_interchange.at(0)) };
		auto devStrs_to_physStrs{ std::move(string_interchange.at(1)) };

		std::cout << std::format("sizeof(Tile_Based_Router::Tile_Info) {}\n", sizeof(Tile_Based_Router::Tile_Info));

		return Tile_Based_Router(
		//host side
			std::move(tile_id_to_shader_tile_id),
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
};
static_assert(sizeof(Tile_Based_Router::Tile_Info) == sizeof(std::array<uint32_t, 8>));

#endif