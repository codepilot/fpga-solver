#pragma once

#include "lib_dev_tile_index.h"
#include "lib_inverse_wires.h"
#include "lib_site_pin_to_wire.h"
#include "lib_site_pin_to_node.h"

#if 1
class Tile_Based_Router {
public:
	static inline const DevFlat devFlat{ TimerVal(DevFlat("_deps/device-file-build/xcvu3p.device")) };

	static inline const device_reader dev{ devFlat.root };
	static inline const tile_list_reader tiles{ dev.getTileList() };
	static inline const tile_type_list_reader tile_types{ dev.getTileTypeList() };
	static inline const wire_list_reader wires{ dev.getWires() };
	static inline const node_list_reader nodes{ dev.getNodes() };
	static inline const string_list_reader devStrs{ dev.getStrList() };

	struct tile_idx_tile_wire_idx {
		uint32_t tile_idx, tile_wire_idx;
	};
	static std::vector<tile_idx_tile_wire_idx> make_wire_idx_to_tile_idx_tile_wire_idx() {
		std::vector<tile_idx_tile_wire_idx> wire_idx_to_tile_idx_tile_wire_idx(static_cast<size_t>(wires.size()), tile_idx_tile_wire_idx{ .tile_idx{UINT32_MAX}, .tile_wire_idx{UINT32_MAX} });
		jthread_each(tiles, [&](uint64_t tile_idx, tile_reader& tile) {
			auto tile_str_idx{ tile.getName() };
			auto tile_type{ tile_types[tile.getType()] };
			auto tile_wires{ tile_type.getWires() };
			auto tile_inverse_wires{ xcvu3p::inverse_wires.tile_range(tile_str_idx) };
			each<uint64_t, decltype(tile_wires), uint32_t>(tile_wires, [&](uint64_t tile_wire_idx, uint32_t wire_str_idx) {
				auto v_wire_idx{ tile_inverse_wires.at(tile_str_idx, wire_str_idx) };
				if (v_wire_idx.size() != 1) abort();
				auto wire_idx{ v_wire_idx.front() };
				wire_idx_to_tile_idx_tile_wire_idx[wire_idx] = tile_idx_tile_wire_idx{ .tile_idx{static_cast<uint32_t>(tile_idx)}, .tile_wire_idx{static_cast<uint32_t>(tile_wire_idx)} };
			});
		});
		return wire_idx_to_tile_idx_tile_wire_idx;
	}

	static inline const std::vector<tile_idx_tile_wire_idx> wire_idx_to_tile_idx_tile_wire_idx = make_wire_idx_to_tile_idx_tile_wire_idx();

	static inline const size_t bans_per_tile{ 100ull };

	class Tile_Info {
	public:
		uint32_t tile_id, tile_wire_offset, inbox_offset, ban_offset;
		uint16_t inbox_read_pos, inbox_write_pos, tile_wire_count, inbox_count, ban_count;
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
	std::vector<uint32_t> tile_id_to_shader_tile_id;
	std::vector<uint32_t> physStrs_to_devStrs;
	std::vector<uint32_t> devStrs_to_physStrs;

//shader side
	std::vector<Tile_Info> tile_infos;
	std::vector<Tile_Wire> tile_wires;
	std::vector<Inbox_Item> inbox_items;
	std::vector<Ban> bans;
	phys_reader phys;
	net_list_reader nets;

	template<bool is_source>
	void block_site_pin(uint32_t net_idx, phys_site_pin_reader sitePin) {
		auto ps_source_site{ sitePin.getSite() };
		auto ps_source_pin{ sitePin.getPin() };
		// OutputDebugStringA(std::format("block_site_pin({}, {})\n", strList[ps_source_site].cStr(), strList[ps_source_pin].cStr()).c_str());
		auto ds_source_site{ physStrs_to_devStrs[ps_source_site] };
		auto ds_source_pin{ physStrs_to_devStrs[ps_source_pin] };
		// OutputDebugStringA(std::format("block_site_pin({}, {})\n", dev.strList[ds_source_site].cStr(), dev.strList[ds_source_pin].cStr()).c_str());

		// source_site_str_idx(19 bit), ds_source_pin_str_idx(19 bit) => wire_idx(24)
		if (is_source) {
			auto source_node_idx{ xcvu3p::site_pin_to_node.at(ds_source_site, ds_source_pin).front() };
			for (auto wire_idx : nodes[source_node_idx].getWires()) {
				decltype(auto) tw{ wire_idx_to_tile_idx_tile_wire_idx[wire_idx] };
				auto shader_tile_id{ tile_id_to_shader_tile_id[tw.tile_idx] };
				decltype(auto) tile_info{ tile_infos[shader_tile_id] };
				auto stw{ std::span(tile_wires).subspan(tile_info.tile_wire_offset, tile_info.tile_wire_count) };
				stw[tw.tile_wire_idx] = Tile_Wire{ .net_id{net_idx}, .previous_tile_col{UINT16_MAX}, .previous_tile_row{UINT16_MAX}, .previous_tile_wire{UINT16_MAX} };
			}

		} else {
		}
#if 0
		s_node_nets[source_node_idx] = net_idx;
#else
		auto wire_idx{ xcvu3p::site_pin_to_wire.at(ds_source_site, ds_source_pin) };
		if (wire_idx.empty()) abort();
		decltype(auto) tw{ wire_idx_to_tile_idx_tile_wire_idx[wire_idx.front()]};
#ifdef _DEBUG
		// std::cout << std::format("block_site_pin({},{},{})\n", wire_idx, tw.tile_idx, tw.tile_wire_idx);
#endif
#endif
	}

	template<bool is_source>
	void block_pip(uint32_t net_idx, ::PhysicalNetlist::PhysNetlist::PhysPIP::Reader pip) {
		auto ps_tile{ pip.getTile() };
		auto ps_wire0{ pip.getWire0() };
		auto ps_wire1{ pip.getWire1() };

		auto ds_tile{ physStrs_to_devStrs[ps_tile] };
		auto ds_wire0{ physStrs_to_devStrs[ps_wire0] };
		auto ds_wire1{ physStrs_to_devStrs[ps_wire1] };

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

	static Tile_Based_Router make(phys_reader phys) {
		std::atomic<uint32_t> needed_tiles, needed_tile_wires, needed_inbox_items, needed_bans;
		jthread_each(tiles, [&](uint64_t tile_idx, tile_reader& tile) {
			auto tile_type{ tile_types[tile.getType()] };
			auto tile_wires{ tile_type.getWires() };
			auto tile_pips{ tile_type.getPips() };
			if (!tile_pips.size()) return;
			needed_tiles++;
			needed_tile_wires += tile_wires.size();// fixme only need inbound wires
			needed_inbox_items += tile_wires.size();// fixme only need inbound wires
			needed_bans += bans_per_tile; //fixme find what works, runs out, etc
		});

		auto tile_infos{ std::vector<Tile_Info>(static_cast<size_t>(needed_tiles.load()), Tile_Info{}) };
		auto tile_wires{ std::vector<Tile_Wire>(static_cast<size_t>(needed_tile_wires.load()), Tile_Wire{}) };
		auto inbox_items{ std::vector<Inbox_Item>(static_cast<size_t>(needed_inbox_items.load()), Inbox_Item{}) };
		auto bans{ std::vector<Ban>(static_cast<size_t>(needed_bans.load()), Ban{}) };

		std::vector<uint32_t> tile_id_to_shader_tile_id(static_cast<size_t>(tiles.size()), UINT32_MAX);

		std::atomic<uint32_t> offset_tiles, offset_tile_wires, offset_inbox_items, offset_bans;
		jthread_each(tiles, [&](uint64_t tile_idx, tile_reader& tile) {
			auto tile_type{ tile_types[tile.getType()] };
			auto tile_wires{ tile_type.getWires() };
			auto tile_pips{ tile_type.getPips() };
			if (!tile_pips.size()) return;

			auto tile_offset{ offset_tiles.fetch_add(1) };

			decltype(auto) tile_info{ tile_infos[tile_offset] = Tile_Info::make(
				static_cast<uint32_t>(tile_idx),
				offset_tiles, offset_tile_wires, offset_inbox_items, offset_bans,
				tile_wires.size(),
				tile_wires.size(),
				bans_per_tile
			)};
			tile_id_to_shader_tile_id[tile_idx] = tile_offset;
		});

		auto site_locations{ TimerVal(make_site_locations(dev)) };
		const auto devStrs_map{ TimerVal(make_devStrs_map(dev)) };

		string_list_reader physStrs{ phys.getStrList() };
		auto string_interchange{ TimerVal(make_string_interchange(devStrs_map, devStrs, physStrs)) };
		auto physStrs_to_devStrs{ std::move(string_interchange.at(0)) };
		auto devStrs_to_physStrs{ std::move(string_interchange.at(1)) };


		return Tile_Based_Router{
		//host side
			.tile_id_to_shader_tile_id{std::move(tile_id_to_shader_tile_id)},
			.physStrs_to_devStrs{std::move(physStrs_to_devStrs)},
			.devStrs_to_physStrs{std::move(devStrs_to_physStrs)},
		//shader side
			.tile_infos{std::move(tile_infos)},
			.tile_wires{std::move(tile_wires)},
			.inbox_items{std::move(inbox_items)},
			.bans{std::move(bans)},
			.phys{phys},
			.nets{phys.getPhysNets()},
		};
	}
};
#endif