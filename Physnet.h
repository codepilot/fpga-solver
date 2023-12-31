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
	FlatReader<PhysicalNetlist::PhysNetlist> phys;
	decltype(phys.reader.getStrList()) strList;
	std::vector<uint32_t> phys_stridx_to_dev_stridx;
	std::unordered_map<uint32_t, uint32_t> dev_strIdx_to_phys_strIdx;
	std::vector<uint32_t> extra_dev_strIdx;
	std::vector<uint64_t> unrouted_indices;
	std::vector<uint32_t> unrouted_locations;
	std::vector<uint64_t> routed_indices;
	std::unordered_map<uint32_t, uint32_t> location_map;

	uint32_t get_phys_strIdx_from_dev_strIdx(uint32_t dev_strIdx) {
		if (dev_strIdx_to_phys_strIdx.contains(dev_strIdx)) {
			return dev_strIdx_to_phys_strIdx.at(dev_strIdx);
		}
		uint32_t ret{ static_cast<uint32_t>( strList.size() + extra_dev_strIdx.size() ) };
		extra_dev_strIdx.emplace_back( dev_strIdx );
		dev_strIdx_to_phys_strIdx.insert({ dev_strIdx, ret });
		return ret;
	}

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

	std::optional<branch_reader> source_site_pin(uint32_t nameIdx, branch_reader branch) {
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

	uint32_t fully_routed{};
	std::unordered_set<uint32_t> stored_wires{};
	std::vector<bool> stored_nodes{};

	DECLSPEC_NOINLINE void store_route(branch_builder branch, branch_reader stub, route_options &ro, uint32_t route_index) {
		std::vector<uint32_t> route_ids{};
		route_ids.reserve(ro.storage[route_index].get_past_cost());
		for (uint32_t current_route_index{ route_index }; current_route_index != 0; current_route_index = ro.storage[current_route_index].get_previous()) {
			route_ids.emplace_back(current_route_index);
			// OutputDebugStringA(std::format("current_route_index: {}, past_cost: {}\n", current_route_index, ro.storage[current_route_index].get_past_cost()).c_str());
		}
		// OutputDebugStringA(std::format("route_ids.size: {}\n", route_ids.size()).c_str());

		auto current_branches{ branch.initBranches(1) };
		for (auto&& it{ route_ids.crbegin() }; it != route_ids.crend(); it++) {
			auto current_route_index{ *it };
			// OutputDebugStringA(std::format("current_route_index: {}, past_cost: {}\n", current_route_index, ro.storage[current_route_index].get_past_cost()).c_str());

			//OutputDebugStringA("Wire match\n");

			stored_wires.insert(ro.storage[current_route_index].get_wire0_idx());
			stored_wires.insert(ro.storage[current_route_index].get_wire1_idx());

			stored_nodes[dev.wire_to_node[ro.storage[current_route_index].get_wire0_idx()]] = true;
			stored_nodes[dev.wire_to_node[ro.storage[current_route_index].get_wire1_idx()]] = true;

			auto psi_tile{ get_phys_strIdx_from_dev_strIdx(dev.wires[ro.storage[current_route_index].get_wire0_idx()].getTile()) };
			auto psi_wire0{ get_phys_strIdx_from_dev_strIdx(dev.wires[ro.storage[current_route_index].get_wire0_idx()].getWire()) };
			auto psi_wire1{ get_phys_strIdx_from_dev_strIdx(dev.wires[ro.storage[current_route_index].get_wire1_idx()].getWire()) };
			// return;
			auto sub_rs{ current_branches[0].initRouteSegment()};
			auto sub_pip{ sub_rs.initPip() };

			sub_pip.setTile(psi_tile);
			sub_pip.setWire0(psi_wire0);
			sub_pip.setWire1(psi_wire1);
			sub_pip.setForward(true);
			sub_pip.setIsFixed(false);

			current_branches = current_branches[0].initBranches(1);

		}

		current_branches.setWithCaveats(0, stub);
		fully_routed++;
		// OutputDebugStringA(std::format("fully_routed: {}, route_ids.size: {}\n", fully_routed, route_ids.size()).c_str());

	}

	DECLSPEC_NOINLINE bool route_stub(branch_builder branch, branch_reader stub, uint32_t source_wire_idx, uint32_t stub_wire_idx, std::vector<uint32_t> stub_node_tiles) {
		route_options ro;
		ro.append(source_wire_idx, source_wire_idx, 0, 0, 0xffff);
		std::unordered_set<uint32_t> used_wires{};
		auto stub_node_idx{ dev.wire_to_node[stub_wire_idx] };

		for (uint32_t attempts{}; attempts < 1000; attempts++) {
			auto top{ ro.q5.top() };
			auto top_info{ ro.storage[top] };

#if 0
			OutputDebugStringA(std::format("attempts: {}, count:{}, topID:{}, wire0:{}, wire1:{}, p:{}, f:{}, t:{}\n",
				attempts, ro.q5.size(), top,
				top_info.get_wire0_idx(),
				top_info.get_wire1_idx(),
				top_info.get_past_cost(),
				top_info.get_future_cost(),
				top_info.get_total_cost()
			).c_str());
#endif

			ro.q5.pop();
			auto current_wire0{ top_info.get_wire1_idx() };
			used_wires.insert(current_wire0);
			auto current_node0{ dev.wire_to_node[current_wire0] };
			auto current_pips{ dev.vv_node_idx_pip_wire_idx_wire_idx[current_node0] };

			for (auto&& source_pip : current_pips) {
				ULARGE_INTEGER v{ .QuadPart{source_pip} };
				auto wire_in{ v.LowPart };
				auto wire_out{ v.HighPart };
				if (used_wires.contains(wire_out)) continue;
				if (stored_wires.contains(wire_out)) continue;

				auto node_out{ dev.wire_to_node[wire_out] };

				if (node_out == stub_node_idx) {
					auto rs_index{ ro.append(wire_in, wire_out, top, top_info.get_past_cost() + 1, 0) };
					store_route(branch, stub, ro, rs_index);

					return true;
				}

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
			}
		}
		// OutputDebugStringA(std::format("count:{}, topID:{}, total_cost:{} fail\n", ro.q5.size(), ro.q5.top(), ro.storage[ro.q5.top()].get_total_cost()).c_str());
		// OutputDebugStringA("\n\n");
		return false;
	}

	DECLSPEC_NOINLINE void block_resources(PhysicalNetlist::PhysNetlist::PhysNet::Reader physNet) {
		for (auto&& src_branch : physNet.getSources()) {
			auto site_pin_optional{ source_site_pin(physNet.getName(), src_branch)};
			if (!site_pin_optional.has_value()) {
//				OutputDebugStringA(std::format("source lacking site_pin {}\n", strList[physNet.getName()].cStr()).c_str());
				continue;
				DebugBreak();
			}
			auto site_pin{ site_pin_optional.value() };
			if (site_pin.getBranches().size()) {
				OutputDebugStringA("site_pin.getBranches().size()\n");
				DebugBreak();
			}
			auto ps_source_site{ site_pin.getRouteSegment().getSitePin().getSite() };
			auto ps_source_pin{ site_pin.getRouteSegment().getSitePin().getPin() };
			auto ds_source_site{ phys_stridx_to_dev_stridx[ps_source_site] };
			auto ds_source_pin{ phys_stridx_to_dev_stridx[ps_source_pin] };
			auto source_wire_idx{ dev.get_site_pin_wire(ds_source_site, ds_source_pin) };
			auto source_node_idx{ dev.wire_to_node[source_wire_idx] };
			stored_nodes[source_node_idx] = true;
		}
		for (auto&& stub_branch : physNet.getStubs()) {
			if (!stub_branch.getRouteSegment().isSitePin()) {
//				OutputDebugStringA(std::format("not stub site pin {}\n", strList[physNet.getName()].cStr()).c_str());
				continue;
				DebugBreak();
			}
			auto ps_stub_site{ stub_branch.getRouteSegment().getSitePin().getSite() };
			auto ps_stub_pin{ stub_branch.getRouteSegment().getSitePin().getPin() };
			auto ds_stub_site{ phys_stridx_to_dev_stridx[ps_stub_site] };
			auto ds_stub_pin{ phys_stridx_to_dev_stridx[ps_stub_pin] };
			auto stub_wire_idx{ dev.get_site_pin_wire(ds_stub_site, ds_stub_pin) };
			auto stub_tile{ dev.tiles[dev.tile_strIndex_to_tile.at(dev.wires[stub_wire_idx].getTile())] };

			auto stub_node_idx{ dev.wire_to_node[stub_wire_idx] };
			stored_nodes[stub_node_idx] = true;
		}



	}

	DECLSPEC_NOINLINE bool assign_stub(uint32_t nameIdx, branch_builder branch, branch_reader stub) {
		// std::vector<route_option> route_options;
		// auto route_option_cmp = [&](uint32_t left, uint32_t right) -> bool { return route_options[left].get_total_cost() < route_options[right].get_total_cost(); };
		// std::priority_queue<uint32_t, std::vector<uint32_t>, decltype(route_option_cmp)> q5(route_option_cmp);

		// return;
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

		auto source_wire_idx{ dev.get_site_pin_wire(ds_source_site, ds_source_pin) };
		auto stub_wire_idx{ dev.get_site_pin_wire(ds_stub_site, ds_stub_pin) };
		auto stub_tile{ dev.tiles[dev.tile_strIndex_to_tile.at(dev.wires[stub_wire_idx].getTile())] };

		auto source_node_idx{ dev.wire_to_node[source_wire_idx] };
		auto stub_node_idx{ dev.wire_to_node[stub_wire_idx] };
		auto stub_node_wires{ dev.nodes[stub_node_idx].getWires() };
		std::vector<uint32_t> stub_node_tiles{};
		stub_node_tiles.reserve(stub_node_wires.size());

		for (auto&& stub_wire_idx : stub_node_wires) {
			auto stub_node_wire{dev.wires[stub_wire_idx]};
			auto stub_node_wire_tile_strIdx{ stub_node_wire.getTile() };
			auto stub_node_wire_tile{ dev.tile_strIndex_to_tile.at(stub_node_wire_tile_strIdx) };
			stub_node_tiles.push_back(stub_node_wire_tile);
		}

		return route_stub(branch, stub, source_wire_idx, stub_wire_idx, stub_node_tiles);
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
			::capnp::MallocMessageBuilder message{/* words */};

			PhysicalNetlist::PhysNetlist::Builder phyBuilder{ message.initRoot<PhysicalNetlist::PhysNetlist>() };

			phyBuilder.setPart(phys.reader.getPart());
			phyBuilder.setPlacements(phys.reader.getPlacements());
#if 1
			auto readerPhysNets{ phys.reader.getPhysNets() };
			uint32_t readerPhysNets_size{ readerPhysNets.size() };
			phyBuilder.initPhysNets(readerPhysNets_size);
			auto listPhysNets{ phyBuilder.getPhysNets() };
			for (auto && phyNetReader: readerPhysNets) {
				block_resources(phyNetReader);
			}
			for (uint32_t n{}; n != readerPhysNets_size; n++) {
				auto phyNetReader{ readerPhysNets[n] };
#if 1
				auto phyNetBuilder{ listPhysNets[n] };
				phyNetBuilder.setName(phyNetReader.getName());
				if (phyNetReader.getSources().size() == 1ui32 && phyNetReader.getStubs().size() == 1ui32) {
					phyNetBuilder.setSources(phyNetReader.getSources());
					// assign_stubs(phyNetBuilder.getSources(), phyNetReader.getStubs());
					if (!assign_stub(phyNetReader.getName(), phyNetBuilder.getSources()[0], phyNetReader.getStubs()[0])) {
						phyNetBuilder.setStubs(phyNetReader.getStubs());
					}
					//phyNetBuilder.initStubs(0);
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
			//phyBuilder.setStrList(phys.reader.getStrList());
			auto strListBuilder{ phyBuilder.initStrList(static_cast<uint32_t>(strList.size() + extra_dev_strIdx.size())) };

			for (uint32_t strIdx{}; strIdx < strList.size(); strIdx++) {
				strListBuilder.set(strIdx, strList[strIdx]);
			}

			for (uint32_t extraIdx{}; extraIdx < extra_dev_strIdx.size(); extraIdx++) {
				auto dev_strIdx{ extra_dev_strIdx[extraIdx] };
				strListBuilder.set(strList.size() + extraIdx, dev.strList[dev_strIdx]);
			}
#if 0
			for (auto&& d_to_p : dev_strIdx_to_phys_strIdx) {
				strListBuilder.set(d_to_p.second, dev.strList[d_to_p.first]);
			}
#endif

			phyBuilder.setSiteInsts(phys.reader.getSiteInsts());
			phyBuilder.setProperties(phys.reader.getProperties());
			phyBuilder.setNullNet(phys.reader.getNullNet());

			auto fa{ messageToFlatArray(message) };
			OutputDebugStringA(std::format("fully_routed: {}\n", fully_routed).c_str());
			auto fa_bytes{ fa.asBytes() };
			msgSize = fa_bytes.size();
			//memcpy(dst_written.fp, fa.begin(), msgSize);
			//kj::ArrayOutputStream aos(;

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
		mmf{ L"benchmarks/boom_soc_unrouted.phys.UNZIPPED" },
//		mmf{ L"_deps/benchmark-files-src/boom_soc_unrouted.phys" },
#else
		mmf{ L"dst-canon.phy" },
#endif
		phys{ mmf },
		strList{ phys.reader.getStrList() },
		phys_stridx_to_dev_stridx{}
	{
		stored_nodes.resize(dev.nodes_size);

		{
			phys_stridx_to_dev_stridx.reserve(static_cast<size_t>(strList.size()));

			for (auto&& str : strList) {
				std::string_view strv{ str.cStr() };
				if (dev.stringMap.contains(strv)) {
					auto dev_strIdx{ dev.stringMap.at(strv) };
					dev_strIdx_to_phys_strIdx.insert({ dev_strIdx, phys_stridx_to_dev_stridx.size() });
					phys_stridx_to_dev_stridx.emplace_back(dev_strIdx);
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