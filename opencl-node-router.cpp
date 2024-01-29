// #define USE_CPP_INSTEAD_OF_OPENCL

#include "ocl_node_router.h"
#include <chrono>

#include "constexpr_string.h"
#include "Timer.h"

std::expected<ocl::context, ocl::status> create_nondefault_gpus_context() {
	auto platforms{ ocl::platform::get().value() };
	for(auto &platform: platforms) {
		auto default_device{ platform.get_devices<CL_DEVICE_TYPE_DEFAULT>().value().front() };
		auto default_device_type{ default_device.get_info_integral<cl_device_type>(CL_DEVICE_TYPE).value() };

#ifdef _DEBUG
		std::cout << std::format("d: {:x} t: {:x} default_device\n", reinterpret_cast<uintptr_t>(default_device.device), default_device_type);
#endif

		std::vector<ocl::device> non_default_gpus;
		auto maybe_all_gpus{ platform.get_devices<CL_DEVICE_TYPE_GPU>() };
		if (!maybe_all_gpus.has_value()) continue;
		std::vector<ocl::device> all_gpus{ maybe_all_gpus.value() };
		std::ranges::copy_if(all_gpus, std::back_inserter(non_default_gpus), [&](ocl::device dev)->bool { return dev.device != default_device.device; });

#ifdef _DEBUG
		for (auto&& dev : all_gpus) {
			std::cout << std::format("d: {:x} all_gpus\n", reinterpret_cast<uintptr_t>(dev.device));
		}

		for (auto&& dev : non_default_gpus) {
			std::cout << std::format("d: {:x} non_default_gpus\n", reinterpret_cast<uintptr_t>(dev.device));
		}
#endif

		std::array<cl_context_properties, 3> context_properties{ CL_CONTEXT_PLATFORM , std::bit_cast<cl_context_properties>(platform.platform) , 0 };
		auto device_ids{ ocl::device::get_device_ids(non_default_gpus.empty() ? all_gpus : non_default_gpus) };
		return ocl::context::create(context_properties, device_ids);
	};
	return ocl::context::create<CL_DEVICE_TYPE_GPU>();
}

bool route_file(std::string src_phys_file, std::string dst_phys_file) {
	std::cout << std::format("  Routing {}\n", src_phys_file);

	auto dev{ TimerVal(DevFlat("_deps/device-file-src/xcvu3p.device")) };
	auto phys{ TimerVal(PhysGZ(src_phys_file)) };


#ifndef USE_CPP_INSTEAD_OF_OPENCL

	auto ocltr{ TimerVal(OCL_Node_Router::make(dev.root, phys.root, create_nondefault_gpus_context().value())) };
#endif

	TimerVal(ocltr.gpu_route()).value();
	::capnp::MallocMessageBuilder message;
	auto success{ TimerVal(ocltr.inspect(message)) };
	TimerVal(InterchangeGZ<PhysicalNetlist::PhysNetlist>::write(dst_phys_file, message));
	return true;
}

int main(int argc, char* argv[]) {
	std::vector<std::string> args;
	for (auto &&arg: std::span<char*>(argv, static_cast<size_t>(argc))) args.emplace_back(arg);

	auto src_phys_file{ (args.size() >= 2) ? args.at(1) : "_deps/benchmark-files-src/boom_med_pb_unrouted.phys" };
	auto dst_phys_file{ (args.size() >= 3) ? args.at(2) : "dst_written.phy.gz" };
	

	TimerVal(route_file(src_phys_file, dst_phys_file));

	return 0;
}