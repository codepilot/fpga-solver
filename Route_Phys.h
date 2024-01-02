#pragma once

#include "DeviceResources.capnp.h"
#include "LogicalNetlist.capnp.h"
#include "PhysicalNetlist.capnp.h"

class Route_Phys {
	using branch = ::PhysicalNetlist::PhysNetlist::RouteBranch;
	using branch_reader = ::PhysicalNetlist::PhysNetlist::RouteBranch::Reader;
	using branch_builder = ::PhysicalNetlist::PhysNetlist::RouteBranch::Builder;
	using branch_list = ::capnp::List<branch, ::capnp::Kind::STRUCT>;
	using branch_list_builder = branch_list::Builder;
	using branch_list_reader = branch_list::Reader;

public:
	DevGZ dev{ "_deps/device-file-src/xcvu3p.device" };
	PhysGZ phys{ "_deps/benchmark-files-src/boom_soc_unrouted.phys" };
	DeviceResources::Device::Reader devRoot{ dev.root };
	PhysicalNetlist::PhysNetlist::Reader physRoot{ phys.root };
	::capnp::List< ::capnp::Text, ::capnp::Kind::BLOB>::Reader devStrs{ devRoot.getStrList() };
	::capnp::List< ::capnp::Text, ::capnp::Kind::BLOB>::Reader physStrs{ physRoot.getStrList() };
	std::vector<uint32_t> extra_dev_strIdx;
	uint32_t fully_routed{};
	uint32_t failed_route{};
	std::vector<bool> stored_nodes{};

	RenumberedWires rw;
	::capnp::MallocMessageBuilder message{/* words */ };
	PhysicalNetlist::PhysNetlist::Builder physBuilder{ message.initRoot<PhysicalNetlist::PhysNetlist>() };

	void block_resources(PhysicalNetlist::PhysNetlist::PhysNet::Reader physNet) {

	}

	bool assign_stub(uint32_t nameIdx, branch_builder branch, branch_reader stub) {
		return false;
	}

	void check_conflicts(PhysicalNetlist::PhysNetlist::Builder physBuilder) {

	}

	void route() {
		physBuilder.setPart(physRoot.getPart());
		physBuilder.setPlacements(physRoot.getPlacements());
		auto readerPhysNets{ physRoot.getPhysNets() };
		uint32_t readerPhysNets_size{ readerPhysNets.size() };
		physBuilder.initPhysNets(readerPhysNets_size);
		auto listPhysNets{ physBuilder.getPhysNets() };
		for (auto&& phyNetReader : readerPhysNets) {
			block_resources(phyNetReader);
		}
		for (uint32_t n{}; n != readerPhysNets_size; n++) {
			auto phyNetReader{ readerPhysNets[n] };
			auto phyNetBuilder{ listPhysNets[n] };
			phyNetBuilder.setName(phyNetReader.getName());
			if (phyNetReader.getSources().size() == 1u && phyNetReader.getStubs().size() == 1u) {
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
		}

		physBuilder.setPhysCells(physRoot.getPhysCells());
		//phyBuilder.setStrList(phys.reader.getStrList());
		auto strListBuilder{ physBuilder.initStrList(static_cast<uint32_t>(physStrs.size() + extra_dev_strIdx.size())) };

		for (uint32_t strIdx{}; strIdx < physStrs.size(); strIdx++) {
			strListBuilder.set(strIdx, physStrs[strIdx]);
		}

		for (uint32_t extraIdx{}; extraIdx < extra_dev_strIdx.size(); extraIdx++) {
			auto dev_strIdx{ extra_dev_strIdx[extraIdx] };
			strListBuilder.set(physStrs.size() + extraIdx, devStrs[dev_strIdx]);
		}
		physBuilder.setSiteInsts(physRoot.getSiteInsts());
		physBuilder.setProperties(physRoot.getProperties());
		physBuilder.setNullNet(physRoot.getNullNet());

		check_conflicts(physBuilder);
		// debug_net(phyBuilder, "system/clint/time_00[5]");
		phys.write(message);
	}
};

