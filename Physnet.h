#pragma once

#include "canon_reader.h"

class Physnet {
public:

	Dev& dev;
	MemoryMappedFile mmf;
	CanonReader<PhysicalNetlist::PhysNetlist> phys;
	decltype(phys.reader.getStrList()) strList;
	std::vector<uint32_t> phys_stridx_to_dev_stridx;
	std::vector<uint64_t> unrouted_indices;
	std::vector<uint32_t> unrouted_locations;
	std::unordered_map<uint32_t, uint32_t> location_map;

	__forceinline constexpr uint32_t get_dev_strIdx(uint32_t phys_strIdx) const noexcept {
		return phys_stridx_to_dev_stridx[phys_strIdx];
	}

	__forceinline uint32_t get_tile_location(uint32_t tile_strIdx) const noexcept {
		auto dev_strIdx{ get_dev_strIdx(tile_strIdx) };
		auto tile_index{ dev.tile_strIndex_to_tile[dev_strIdx] };
		auto tile{ dev.tiles[tile_index] };
		std::array<uint16_t, 2> tile_location{ tile.getCol(), tile.getRow() };
		return std::bit_cast<uint32_t>(tile_location);
	}

	__forceinline uint32_t get_site_location(uint32_t site_strIdx) const noexcept {
		auto dev_strIdx{ get_dev_strIdx(site_strIdx) };
		auto tile_index{ dev.site_strIndex_to_tile[dev_strIdx] };
		auto tile{ dev.tiles[tile_index] };
		std::array<uint16_t, 2> tile_location{ tile.getCol(), tile.getRow() };
		return std::bit_cast<uint32_t>(tile_location);
	}

	__forceinline uint32_t get_belpin_location(::PhysicalNetlist::PhysNetlist::PhysBelPin::Reader belpin) const noexcept {
		return get_site_location(belpin.getSite());
	}

	__forceinline uint32_t get_sitepin_location(::PhysicalNetlist::PhysNetlist::PhysSitePin::Reader sitepin) const noexcept {
		return get_site_location(sitepin.getSite());
	}

	__forceinline uint32_t get_pip_location(::PhysicalNetlist::PhysNetlist::PhysPIP::Reader pip) const noexcept {
		return get_tile_location(pip.getTile());
	}

	__forceinline uint32_t get_sitepip_location(::PhysicalNetlist::PhysNetlist::PhysSitePIP::Reader sitepip) const noexcept {
		return get_site_location(sitepip.getSite());
	}

	__forceinline uint32_t get_rs_location(PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::Reader rs) const noexcept {
		switch (rs.which()) {
		case PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::Which::BEL_PIN: return get_belpin_location(rs.getBelPin());
		case PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::Which::SITE_PIN: return get_sitepin_location(rs.getSitePin());
		case PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::Which::PIP: return get_pip_location(rs.getPip());
		case PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::Which::SITE_P_I_P: return get_sitepip_location(rs.getSitePIP());
		}
		std::unreachable();
	}

	__forceinline uint32_t get_location_index(uint32_t location) noexcept {
		auto found_index{ location_map.find(location) };
		if (found_index != location_map.end()) {
			return found_index->second;
		}
		uint32_t ret{ static_cast<uint32_t>(unrouted_locations.size()) };
		unrouted_locations.emplace_back(location);
		location_map.emplace(location, ret);
		return ret;
	}

	DECLSPEC_NOINLINE Physnet(Dev& dev) noexcept :
		dev{ dev },
		mmf{ L"benchmarks/boom_soc_unrouted.phys" },
		phys{ mmf },
		strList{ phys.reader.getStrList() },
		phys_stridx_to_dev_stridx{}
	{
		{
			phys_stridx_to_dev_stridx.reserve(static_cast<size_t>(strList.size()));
			for (auto&& str : strList) {
				std::string_view strv{ str.cStr() };
				if (dev.stringMap.contains(strv)) {
					phys_stridx_to_dev_stridx.emplace_back(dev.stringMap.at(strv));
				}
				else {
					phys_stridx_to_dev_stridx.emplace_back(UINT32_MAX);
				}
			}
		}
		{
			for (auto&& physNet : phys.reader.getPhysNets()) {
				auto sources{ physNet.getSources() };
				if (sources.size() > 1) {
					OutputDebugStringA(std::format("{}({})\n", strList[physNet.getName()].cStr(), sources.size()).c_str());
					continue;
				}
				for (auto&& source : sources) {
					auto source_location{ get_rs_location(source.getRouteSegment()) };
					auto source_location_index{ get_location_index(source_location) };
					for (auto&& stub : physNet.getStubs()) {
						auto stub_location{ get_rs_location(stub.getRouteSegment()) };
						auto stub_location_index{ get_location_index(stub_location) };
						unrouted_indices.emplace_back(ULARGE_INTEGER{ .u{.LowPart{source_location_index}, .HighPart{stub_location_index}} }.QuadPart);
					}
					break;
				}
			}
			location_map.clear();
		}
	}
};