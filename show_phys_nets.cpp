#include <capnp/serialize.h>
#include "DeviceResources.capnp.h"
#include "LogicalNetlist.capnp.h"
#include "PhysicalNetlist.capnp.h"

#include <cstdint>
#include <type_traits>
#include <immintrin.h>
#include <cstdint>
#include <array>
#include <span>
#include <string>
#include <codecvt>
#include <format>
#include <print>
#include <algorithm>

#include "constexpr_intrinsics.h"
#include "constexpr_siphash.h"
#include "Trivial_Span.h"

#include <Windows.h>
#include "MemoryMappedFile.h"

#include <zlib/zlib.h>

DECLSPEC_NOINLINE void debug_belpin(uint32_t indent, PhysicalNetlist::PhysNetlist::Reader physRoot, ::PhysicalNetlist::PhysNetlist::PhysBelPin::Reader belpin) {
	std::print("{:>{}}BelPin\n", "", indent);
}

DECLSPEC_NOINLINE void debug_sitepin(uint32_t indent, PhysicalNetlist::PhysNetlist::Reader physRoot, ::PhysicalNetlist::PhysNetlist::PhysSitePin::Reader sitePin) {
	std::print("{:>{}}SitePin\n", "", indent);
}

DECLSPEC_NOINLINE void debug_pip(uint32_t indent, PhysicalNetlist::PhysNetlist::Reader physRoot, ::PhysicalNetlist::PhysNetlist::PhysPIP::Reader pip) {
	std::print("{:>{}}PIP\n", "", indent);
}

DECLSPEC_NOINLINE void debug_sitepip(uint32_t indent, PhysicalNetlist::PhysNetlist::Reader physRoot, ::PhysicalNetlist::PhysNetlist::PhysSitePIP::Reader sitePip) {
	std::print("{:>{}}SitePIP\n", "", indent);
}

DECLSPEC_NOINLINE void debug_branch(uint32_t indent, PhysicalNetlist::PhysNetlist::Reader physRoot, ::PhysicalNetlist::PhysNetlist::RouteBranch::Reader branch) {
	auto rs{ branch.getRouteSegment() };

	switch (rs.which()) {
	case PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::Which::BEL_PIN: return debug_belpin(indent, physRoot, rs.getBelPin());
	case PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::Which::SITE_PIN: return debug_sitepin(indent, physRoot, rs.getSitePin());
	case PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::Which::PIP: return debug_pip(indent, physRoot, rs.getPip());
	case PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::Which::SITE_P_I_P: return debug_sitepip(indent, physRoot, rs.getSitePIP());
	}
}

DECLSPEC_NOINLINE void debug_branches(uint32_t indent, PhysicalNetlist::PhysNetlist::Reader physRoot, ::capnp::List< ::PhysicalNetlist::PhysNetlist::RouteBranch, ::capnp::Kind::STRUCT>::Reader branches) {
	for (auto&& branch : branches) {
		debug_branch(indent, physRoot, branch);
		debug_branches(indent + 1, physRoot, branch.getBranches());
	}
}

DECLSPEC_NOINLINE void debug_net(PhysicalNetlist::PhysNetlist::Reader physRoot) {
	auto strList{ physRoot.getStrList() };

	std::string_view search_name{ "ram_reg_bram_0_i_51__4_n_0" };

	for (auto&& net : physRoot.getPhysNets()) {
		std::string_view net_name{ strList[net.getName()].cStr() };
		if (search_name != net_name) continue;
		std::print("{}\n", net_name);
		if (net.getSources().size()) {
			std::print(" sources:\n");
			debug_branches(2, physRoot, net.getSources());
		}

		if (net.getStubs().size()) {
			std::print(" stubs:\n");
			debug_branches(2, physRoot, net.getStubs());
		}
	}
}

DECLSPEC_NOINLINE void debug_phys(std::wstring gzPath) {
	MemoryMappedFile mmf_phys_gz{ gzPath };
	MemoryMappedFile mmf_phys{ L"temp_unzipped.bin", 4294967296ui64 };

	auto read_span{ mmf_phys_gz.get_span<Bytef>() };
	auto write_span{ mmf_phys.get_span<Bytef>() };
	z_stream strm{
		.next_in{read_span.data()},
		.avail_in{static_cast<uint32_t>(read_span.size())},
		.next_out{write_span.data()},
		.avail_out{UINT32_MAX},
	};

	auto init_result{ inflateInit2(&strm, 15 + 16) };
	auto inflate_result{ inflate(&strm, Z_FINISH) };
	auto end_result{ inflateEnd(&strm) };
	std::print("init_result: {}\ninflate_result: {}\nend_result: {}\n", init_result, inflate_result, end_result);
	auto mmf_unzipped{ mmf_phys.shrink(strm.total_out) };


	auto span_words{ mmf_unzipped.get_span<capnp::word>() };
	kj::ArrayPtr<capnp::word> words{ span_words.data(), span_words.size() };
	capnp::FlatArrayMessageReader famr{ words, {.traversalLimitInWords = UINT64_MAX, .nestingLimit = INT32_MAX} };
	auto phys{ famr.getRoot<PhysicalNetlist::PhysNetlist>() };

	debug_net(phys);

	mmf_unzipped.reopen_delete();
}

int main(int argc, char* argv[]) {
	debug_phys(L"_deps/benchmark-files-src/boom_soc_unrouted.phys");
	debug_phys(L"dst_written.phy.gz");
	return 0;
}