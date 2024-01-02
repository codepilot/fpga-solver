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
#ifdef _WIN32
#include <print>
#endif
#include <algorithm>

#ifdef _WIN32
#include "constexpr_intrinsics.h"
#include "constexpr_siphash.h"
#endif

#include "Trivial_Span.h"

#ifdef __GNUC__
#elif defined(_WIN32)
#include <Windows.h>
#endif

#include "MemoryMappedFile.h"

#include <zlib/zlib.h>

void debug_belpin(uint32_t indent, PhysicalNetlist::PhysNetlist::Reader physRoot, ::PhysicalNetlist::PhysNetlist::PhysBelPin::Reader belpin) {
	puts(std::format("{:>{}}BelPin", "", indent).c_str());
}

void debug_sitepin(uint32_t indent, PhysicalNetlist::PhysNetlist::Reader physRoot, ::PhysicalNetlist::PhysNetlist::PhysSitePin::Reader sitePin) {
	puts(std::format("{:>{}}SitePin", "", indent).c_str());
}

void debug_pip(uint32_t indent, PhysicalNetlist::PhysNetlist::Reader physRoot, ::PhysicalNetlist::PhysNetlist::PhysPIP::Reader pip) {
	puts(std::format("{:>{}}PIP", "", indent).c_str());
}

void debug_sitepip(uint32_t indent, PhysicalNetlist::PhysNetlist::Reader physRoot, ::PhysicalNetlist::PhysNetlist::PhysSitePIP::Reader sitePip) {
	puts(std::format("{:>{}}SitePIP", "", indent).c_str());
}

void debug_branch(uint32_t indent, PhysicalNetlist::PhysNetlist::Reader physRoot, ::PhysicalNetlist::PhysNetlist::RouteBranch::Reader branch) {
	auto rs{ branch.getRouteSegment() };

	switch (rs.which()) {
	case PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::Which::BEL_PIN: return debug_belpin(indent, physRoot, rs.getBelPin());
	case PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::Which::SITE_PIN: return debug_sitepin(indent, physRoot, rs.getSitePin());
	case PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::Which::PIP: return debug_pip(indent, physRoot, rs.getPip());
	case PhysicalNetlist::PhysNetlist::RouteBranch::RouteSegment::Which::SITE_P_I_P: return debug_sitepip(indent, physRoot, rs.getSitePIP());
	}
}

void debug_branches(uint32_t indent, PhysicalNetlist::PhysNetlist::Reader physRoot, ::capnp::List< ::PhysicalNetlist::PhysNetlist::RouteBranch, ::capnp::Kind::STRUCT>::Reader branches) {
	for (auto&& branch : branches) {
		debug_branch(indent, physRoot, branch);
		debug_branches(indent + 1, physRoot, branch.getBranches());
	}
}

void debug_net(PhysicalNetlist::PhysNetlist::Reader physRoot) {
	auto strList{ physRoot.getStrList() };

	std::string_view search_name{ "ram_reg_bram_0_i_51__4_n_0" };

	for (auto&& net : physRoot.getPhysNets()) {
		std::string_view net_name{ strList[net.getName()].cStr() };
		if (search_name != net_name) continue;
		puts(std::format("{}", net_name).c_str());
		if (net.getSources().size()) {
			puts(std::format(" sources:").c_str());
			debug_branches(2, physRoot, net.getSources());
		}

		if (net.getStubs().size()) {
			puts(std::format(" stubs:").c_str());
			debug_branches(2, physRoot, net.getStubs());
		}
	}
}

void debug_phys(std::string gzPath) {
	MemoryMappedFile mmf_phys_gz{ gzPath };
	MemoryMappedFile mmf_phys{ ".", 4294967296ull };

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
	puts(std::format("init_result: {}\ninflate_result: {}\nend_result: {}\n", init_result, inflate_result, end_result).c_str());


	auto span_words{ mmf_phys.get_span<capnp::word>().subspan(0, static_cast<size_t>(strm.total_out) / sizeof(capnp::word)) };
	kj::ArrayPtr<capnp::word> words{ span_words.data(), span_words.size() };
	capnp::FlatArrayMessageReader famr{ words, {.traversalLimitInWords = UINT64_MAX, .nestingLimit = INT32_MAX} };
	auto phys{ famr.getRoot<PhysicalNetlist::PhysNetlist>() };

	debug_net(phys);

}

int main(int argc, char* argv[]) {
	debug_phys("_deps/benchmark-files-src/boom_soc_unrouted.phys");
	// debug_phys("dst_written.phy.gz");
	return 0;
}