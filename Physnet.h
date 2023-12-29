#pragma once

#include "canon_reader.h"

#define REBUILD_PHYS

using branch = ::PhysicalNetlist::PhysNetlist::RouteBranch;
using branch_reader = ::PhysicalNetlist::PhysNetlist::RouteBranch::Reader;
using branch_builder = ::PhysicalNetlist::PhysNetlist::RouteBranch::Builder;
using branch_list = ::capnp::List<branch, ::capnp::Kind::STRUCT>;
using branch_list_builder = branch_list::Builder;
using branch_list_reader = branch_list::Reader;

class Physnet {
public:

	Dev& dev;
	MemoryMappedFile mmf;
	CanonReader<PhysicalNetlist::PhysNetlist> phys;
	decltype(phys.reader.getStrList()) strList;
	std::vector<uint32_t> phys_stridx_to_dev_stridx;
	std::vector<uint64_t> unrouted_indices;
	std::vector<uint32_t> unrouted_locations;
	std::vector<uint64_t> routed_indices;
	std::unordered_map<uint32_t, uint32_t> location_map;

	std::optional<branch_builder> source_site_pin(uint32_t nameIdx, branch_builder branch) {
		auto name{ strList[nameIdx] };
		auto branch_rs{ branch.getRouteSegment() };
		auto branch_branches{ branch.getBranches() };
		auto branch_rs_which{ branch_rs.which() };
		// OutputDebugStringA(std::format("{} branches({}) which {}\n", name.cStr(), branch_branches.size(), static_cast<uint16_t>(branch_rs_which)).c_str());
		switch (branch_rs_which) {
		case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::BEL_PIN: {
			// OutputDebugStringA("BEL_PIN\n");
			if (branch_branches.size()) {
				for (auto&& subbranch : branch_branches) {
					auto ret{ source_site_pin(nameIdx, subbranch) };
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
			return branch;
		}
		case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::PIP: {
			OutputDebugStringA("PIP\n");
			DebugBreak();
			break;
		}
		case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::SITE_P_I_P: {
			// OutputDebugStringA("SITE_P_I_P\n");
			if (branch_branches.size()) {
				for (auto&& subbranch : branch_branches) {
					auto ret{ source_site_pin(nameIdx, subbranch) };
					if (ret.has_value()) return ret;
				}
				return std::nullopt;
			}
			else {
				// OutputDebugStringA("SITE_P_I_P Empty\n");
				return std::nullopt;
			}

			DebugBreak();
			break;
		}
		default: {
			DebugBreak();
			std::unreachable();
		}
		}
		return std::nullopt;
	}

	DECLSPEC_NOINLINE void assign_stub(uint32_t nameIdx, branch_builder branch, branch_reader stub) {
		auto site_pin_optional{ source_site_pin(nameIdx, branch) };
		if (!site_pin_optional.has_value()) {
			OutputDebugStringA("source lacking site_pin\n");
			DebugBreak();
		}
		auto site_pin{ site_pin_optional.value() };
		if (site_pin.getBranches().size()) {
			OutputDebugStringA("site_pin.getBranches().size()\n");
			DebugBreak();
		}
		if (!stub.getRouteSegment().isSitePin()) {
			OutputDebugStringA("not stub site pin\n");
			DebugBreak();
		}
		auto ps_source_site{ site_pin.getRouteSegment().getSitePin().getSite() };
		auto ps_source_pin{ site_pin.getRouteSegment().getSitePin().getPin() };
		auto ps_stub_site{ stub.getRouteSegment().getSitePin().getSite() };
		auto ps_stub_pin{ stub.getRouteSegment().getSitePin().getPin() };
#if 0
		OutputDebugStringA(std::format("source {}:{}, stub {}:{}\n", strList[ps_source_site].cStr(), strList[ps_source_pin].cStr(), strList[ps_stub_site].cStr(), strList[ps_stub_pin].cStr()).c_str());
#endif

		auto ds_source_site{ phys_stridx_to_dev_stridx[ps_source_site] };
		auto ds_source_pin{ phys_stridx_to_dev_stridx[ps_source_pin] };
		auto ds_stub_site{ phys_stridx_to_dev_stridx[ps_stub_site] };
		auto ds_stub_pin{ phys_stridx_to_dev_stridx[ps_stub_pin] };

#if 0
		OutputDebugStringA(std::format("source {}:{}, stub {}:{}\n",
			dev.strList[ds_source_site].cStr(),
			dev.strList[ds_source_pin].cStr(),
			dev.strList[ds_stub_site].cStr(),
			dev.strList[ds_stub_pin].cStr()
		).c_str());
#endif

		dev.get_site_pin_wire(ds_source_site, ds_source_pin);
		dev.get_site_pin_wire(ds_stub_site, ds_stub_pin);

#if 0
		auto name{ strList[nameIdx] };
		auto branch_rs{ branch.getRouteSegment() };
		auto branch_branches{ branch.getBranches() };
		auto branch_rs_which{ branch_rs.which() };
		OutputDebugStringA(std::format("{} branches({}) which {}\n", name.cStr(), branch_branches.size(), static_cast<uint16_t>(branch_rs_which)).c_str());
		switch (branch_rs_which) {
		case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::BEL_PIN: {
			OutputDebugStringA("BEL_PIN\n");
			if (branch_branches.size()) {
				for (auto&& subbranch : branch_branches) {
					assign_stub(nameIdx, subbranch, stub);
				}
			}
			else {
				DebugBreak();
			}
			break;
		}
		case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::SITE_PIN: {
			OutputDebugStringA("SITE_PIN\n");
			DebugBreak();
			break;
		}
		case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::PIP: {
			OutputDebugStringA("PIP\n");
			DebugBreak();
			break;
		}
		case ::PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::SITE_P_I_P: {
			OutputDebugStringA("SITE_P_I_P\n");
			DebugBreak();
			break;
		}
		default: {
			DebugBreak();
			std::unreachable();
		}
		}
#endif
#if 0
		for (auto&& branch : branches) {
			auto branch_location{ get_rs_location(branch.getRouteSegment()) };
			branch_list_builder branch_branches{ branch.getBranches() };
			uint32_t branch_branches_size{ branch_branches.size() };
			if (branch_branches_size == 0) {
				branch.setBranches(stubs);
				return;
			}
			else {
				assign_stubs(branch_branches, stubs);
				return;
			}
			break;
		}
#endif
	}

	void assign_stubs(branch_list_builder branches, branch_list_reader stubs) {
		for (auto&& branch : branches) {
			auto branch_location{ get_rs_location(branch.getRouteSegment()) };
			branch_list_builder branch_branches{ branch.getBranches() };
			uint32_t branch_branches_size{ branch_branches.size() };
			if (branch_branches_size == 0) {
				branch.setBranches(stubs);
				return;
			}
			else {
				assign_stubs(branch_branches, stubs);
				return;
			}
			break;
		}
	}

	DECLSPEC_NOINLINE void build() {
#ifdef REBUILD_PHYS
		MemoryMappedFile dst{ L"dst.phy", 4294967296ui64 };
		MemoryMappedFile dst_written{ L"dst_written.phy.gz", 4294967296ui64 };

		auto span_words{ dst.get_span<capnp::word>() };
		auto span_words_written{ dst.get_span<capnp::word>() };

		kj::ArrayPtr<capnp::word> words{ span_words.data(), span_words.size() - 1ui64 };
		kj::ArrayPtr<capnp::word> words_written{ span_words_written.data(), span_words_written.size() - 1ui64 };

		size_t msgSize{};
		size_t gzSize{};
		{
			::capnp::MallocMessageBuilder message{ words };

			PhysicalNetlist::PhysNetlist::Builder phyBuilder{ message.initRoot<PhysicalNetlist::PhysNetlist>() };

			phyBuilder.setPart(phys.reader.getPart());
			phyBuilder.setPlacements(phys.reader.getPlacements());
#if 1
			auto readerPhysNets{ phys.reader.getPhysNets() };
			uint32_t readerPhysNets_size{ readerPhysNets.size() };
			phyBuilder.initPhysNets(readerPhysNets_size);
			auto listPhysNets{ phyBuilder.getPhysNets() };
			for (uint32_t n{}; n != readerPhysNets_size; n++) {
				auto phyNetReader{ readerPhysNets[n] };
#if 1
				auto phyNetBuilder{ listPhysNets[n] };
				phyNetBuilder.setName(phyNetReader.getName());
				if (phyNetReader.getSources().size() == 1ui32 && phyNetReader.getStubs().size() == 1ui32) {
					phyNetBuilder.setSources(phyNetReader.getSources());
					// assign_stubs(phyNetBuilder.getSources(), phyNetReader.getStubs());
					assign_stub(phyNetReader.getName(), phyNetBuilder.getSources()[0], phyNetReader.getStubs()[0]);
					phyNetBuilder.initStubs(0);
				}
				else {
					phyNetBuilder.setSources(phyNetReader.getSources());
					phyNetBuilder.setStubs(phyNetReader.getStubs());
				}
				phyNetBuilder.setType(phyNetReader.getType());
				phyNetBuilder.setStubNodes(phyNetReader.getStubNodes());
#else
				listPhysNets.setWithCaveats(n, phyNetReader);
				auto phyNetBuilder{ listPhysNets[n] };
				phyNetBuilder.setName(phyNetReader.getName());
				phyNetBuilder.setSources(phyNetReader.getSources());
				//phyNetBuilder.setStubs(phyNetReader.getStubs());
				phyNetBuilder.setType(phyNetReader.getType());
				//phyNetBuilder.setStubNodes(phyNetReader.getStubNodes());
#endif
			}
#else
			phyBuilder.setPhysNets(phys.reader.getPhysNets());
#endif  
			phyBuilder.setPhysCells(phys.reader.getPhysCells());
			phyBuilder.setStrList(phys.reader.getStrList());
			phyBuilder.setSiteInsts(phys.reader.getSiteInsts());
			phyBuilder.setProperties(phys.reader.getProperties());
			phyBuilder.setNullNet(phys.reader.getNullNet());

			auto fa{ messageToFlatArray(message) };
			msgSize = ::capnp::computeSerializedSizeInWords(message) * sizeof(capnp::word);
			//memcpy(dst_written.fp, fa.begin(), msgSize);
			z_stream strm{
				.next_in{reinterpret_cast<Bytef *>(fa.begin())},
				.avail_in{msl::utilities::SafeInt<uint32_t>(msgSize)},
				.next_out{reinterpret_cast<Bytef *>(dst_written.fp)},
				.avail_out{0xffffffffui32},

			};
			deflateInit2(&strm, Z_NO_COMPRESSION, Z_DEFLATED, 16 | 15, 9, Z_DEFAULT_STRATEGY);
			deflate(&strm, Z_FINISH);
			deflateEnd(&strm);

			gzSize = strm.total_out;
		}
		auto shrunk{ dst.shrink(msgSize) };
		auto shrunk_written{ dst_written.shrink(gzSize) };
#if 0
		auto written_span_words{ shrunk_written.get_span<capnp::word>() };
		kj::ArrayPtr<capnp::word> written_words{ written_span_words.data(), written_span_words.size() };
		capnp::FlatArrayMessageReader famr{ written_words, {.traversalLimitInWords = UINT64_MAX, .nestingLimit = INT32_MAX} };
		auto anyReader{ famr.getRoot<capnp::AnyStruct>() };


		MemoryMappedFile mmf_canon_dst{ L"dst-canon.phy", msgSize };
		auto mmf_canon_dst_span{ mmf_canon_dst.get_span<capnp::word>() };
		kj::ArrayPtr<capnp::word> backing{ mmf_canon_dst_span.data(), mmf_canon_dst_span.size() };
		auto canonical_size = anyReader.canonicalize(backing);
		auto mmf_canon_dst_shrunk{ mmf_canon_dst.shrink(canonical_size * sizeof(capnp::word)) };
		//std::print("canon_size:   {}\n", mmf_canon_dst_shrunk.fsize);
#endif
#endif

	}

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

	void draw_routed_tree(branch_reader source) {

		auto source_location{ get_rs_location(source.getRouteSegment()) };
		auto source_location_index{ get_location_index(source_location) };

		for (auto&& branch : source.getBranches()) {
			auto branch_location{ get_rs_location(branch.getRouteSegment()) };
			auto branch_location_index{ get_location_index(branch_location) };
			routed_indices.emplace_back(ULARGE_INTEGER{ .u{.LowPart{source_location_index}, .HighPart{branch_location_index}} }.QuadPart);
			draw_routed_tree(branch);
		}
	}

	DECLSPEC_NOINLINE Physnet(Dev& dev) noexcept :
		dev{ dev },
#ifdef REBUILD_PHYS
		mmf{ L"benchmarks/boom_soc_unrouted.phys" },
#else
		mmf{ L"dst-canon.phy" },
#endif
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
					draw_routed_tree(source);
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