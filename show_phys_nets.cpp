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

#include <zlib.h>

void debug_belpin(uint32_t indent, PhysicalNetlist::PhysNetlist::Reader physRoot, ::PhysicalNetlist::PhysNetlist::PhysBelPin::Reader belpin) {
	puts(std::format("{:>{}}BelPin Site:{} Bel:{} Pin:{}",
		"", indent,
		physRoot.getStrList()[belpin.getSite()].cStr(),
		physRoot.getStrList()[belpin.getBel()].cStr(),
		physRoot.getStrList()[belpin.getPin()].cStr()
	).c_str());
}

void debug_sitepin(uint32_t indent, PhysicalNetlist::PhysNetlist::Reader physRoot, ::PhysicalNetlist::PhysNetlist::PhysSitePin::Reader sitePin) {
	puts(std::format("{:>{}}SitePin Site:{} Pin:{}",
		"", indent,
		physRoot.getStrList()[sitePin.getSite()].cStr(),
		physRoot.getStrList()[sitePin.getPin()].cStr()
	).c_str());
}

void debug_pip(uint32_t indent, PhysicalNetlist::PhysNetlist::Reader physRoot, ::PhysicalNetlist::PhysNetlist::PhysPIP::Reader pip) {
	puts(std::format("{:>{}}PIP tile:{} wire0:{} wire1:{} forward:{}", "", indent,
		physRoot.getStrList()[pip.getTile()].cStr(),
		physRoot.getStrList()[pip.getWire0()].cStr(),
		physRoot.getStrList()[pip.getWire1()].cStr(),
		pip.getForward()
	).c_str());
}

void debug_sitepip(uint32_t indent, PhysicalNetlist::PhysNetlist::Reader physRoot, ::PhysicalNetlist::PhysNetlist::PhysSitePIP::Reader sitePip) {
	puts(std::format("{:>{}}SitePIP Site:{} Bel:{} Pin:{}",
		"", indent,
		physRoot.getStrList()[sitePip.getSite()].cStr(),
		physRoot.getStrList()[sitePip.getBel()].cStr(),
		physRoot.getStrList()[sitePip.getPin()].cStr()
	).c_str());
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

void debug_net(PhysicalNetlist::PhysNetlist::Reader physRootUnrouted, PhysicalNetlist::PhysNetlist::Reader physRootRouted) {
	auto strListUnrouted{ physRootUnrouted.getStrList() };
	auto strListRouted{ physRootRouted.getStrList() };

	auto physNetsUnrouted{ physRootUnrouted.getPhysNets() };
	auto physNetsRouted{ physRootRouted.getPhysNets() };
	auto physNetsUnrouted_size{ physNetsUnrouted.size() };
	uint32_t no_load{};

	// std::string_view search_name{ "ram_reg_bram_0_i_51__4_n_0" };

	for (uint32_t netIdx{}; netIdx != physNetsUnrouted_size; netIdx++) {
		auto netUnrouted{ physNetsUnrouted[netIdx] };
		auto netRouted{ physNetsRouted[netIdx] };
		auto sourcesUnrouted{ netUnrouted.getSources() };
		auto sourcesRouted{ netRouted.getSources() };
		auto stubsUnrouted{ netUnrouted.getStubs() };
		auto stubsRouted{ netRouted.getStubs() };
		if (sourcesUnrouted.size() && !stubsUnrouted.size()) {
			no_load++;
			continue;
		}

		if (stubsRouted.size()) continue;

		std::string_view net_name{ strListRouted[netRouted.getName()].cStr() };
		// if (search_name != net_name) continue;

		puts(std::format("sources:{}, stubs:{}, name: {}", sourcesUnrouted.size(), stubsUnrouted.size(), net_name).c_str());

		if (sourcesUnrouted.size()) {
			puts(std::format(" sources:").c_str());
			debug_branches(2, physRootUnrouted, sourcesUnrouted);
		}

		if (stubsUnrouted.size()) {
			puts(std::format(" stubs:").c_str());
			debug_branches(2, physRootUnrouted, stubsUnrouted);
		}

		puts(std::format("sources:{}, stubs:{}, name: {}", sourcesRouted.size(), stubsRouted.size(), net_name).c_str());

		if (sourcesRouted.size()) {
			puts(std::format(" sources:").c_str());
			debug_branches(2, physRootRouted, sourcesRouted);
		}

		if (stubsRouted.size()) {
			puts(std::format(" stubs:").c_str());
			debug_branches(2, physRootRouted, stubsRouted);
		}


	}
	puts(std::format("no_load: {}", no_load).c_str());
}


#include "InterchangeGZ.h"

void debug_phys(std::string gzPathUnrouted, std::string gzPathRouted) {

	PhysGZ igzUnrouted{ gzPathUnrouted };
	PhysGZ igzRouted{ gzPathRouted };

	debug_net(igzUnrouted.root, igzRouted.root);

}

int main(int argc, char* argv[]) {
	debug_phys("_deps/benchmark-files-src/vtr_mcml_unrouted.phys", "_deps/benchmark-files-build/vtr_mcml.phys");
	return 0;
}