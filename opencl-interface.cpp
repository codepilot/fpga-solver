#include "ocl.h"
#include "MemoryMappedFile.h"

int main() {
	auto context{ocl::context::create<CL_DEVICE_TYPE_GPU>().value()};
	auto devices{ context.get_devices().value() };
	MemoryMappedFile source{ "../kernels/draw_wires.cl" };
	auto program{ context.create_program(source.get_span<char>()).value() };
	auto prog_devices_before_build{ program.get_devices().value() };
	program.build().value();
	auto prog_devices_after_build{ program.get_devices().value() };
	auto build_status{ program.get_build_info_integral<cl_build_status>(CL_PROGRAM_BUILD_STATUS).value() };
	auto build_log{ program.get_build_info_string(CL_PROGRAM_BUILD_LOG).value() };
	auto program_il{ program.get_info_string(CL_PROGRAM_IL).value_or("")};
	auto program_source{ program.get_info_string(CL_PROGRAM_SOURCE).value() };

	auto kernels{ program.create_kernels().value() };
	for (auto &&kernel : kernels) {
		for (auto&& device : prog_devices_after_build) {
			std::cout << kernel.get_info_string(CL_KERNEL_FUNCTION_NAME).value() << std::endl;;
			std::cout << kernel.get_info_string(CL_KERNEL_ATTRIBUTES).value() << std::endl;;
			auto global_work_size{ kernel.get_work_group_info_integral<std::array<size_t, 3>>(device, CL_KERNEL_GLOBAL_WORK_SIZE).value_or(std::array<size_t, 3>{}) };
			auto compile_work_group_size{ kernel.get_work_group_info_integral<std::array<size_t, 3>>(device, CL_KERNEL_COMPILE_WORK_GROUP_SIZE).value() };
			std::cout << std::format("GLOBAL_WORK_SIZE: {} {} {}\n", global_work_size.at(0), global_work_size.at(1), global_work_size.at(2));
			std::cout << std::format("WORK_GROUP_SIZE: {}\n", kernel.get_work_group_info_integral<size_t>(device, CL_KERNEL_WORK_GROUP_SIZE).value());
			std::cout << std::format("COMPILE_WORK_GROUP_SIZE: {} {} {}\n", compile_work_group_size.at(0), compile_work_group_size.at(1), compile_work_group_size.at(2));
			std::cout << std::format("LOCAL_MEM_SIZE: {}\n", kernel.get_work_group_info_integral<cl_ulong>(device, CL_KERNEL_LOCAL_MEM_SIZE).value());
			std::cout << std::format("PREFERRED_WORK_GROUP_SIZE_MULTIPLE: {}\n", kernel.get_work_group_info_integral<size_t>(device, CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE).value());
			std::cout << std::format("PRIVATE_MEM_SIZE: {}\n", kernel.get_work_group_info_integral<cl_ulong>(device, CL_KERNEL_PRIVATE_MEM_SIZE).value());

			auto num_args{ kernel.get_info_integral<cl_uint>(CL_KERNEL_NUM_ARGS).value() };
			for (cl_uint arg_index{}; arg_index < num_args; arg_index++) {
#if 1
				std::cout << std::format("  {} {} {} {} {}\n",
					ocl::ACCESS_QUALIFIER.at(kernel.get_arg_info_integral<cl_kernel_arg_access_qualifier>(arg_index, CL_KERNEL_ARG_ACCESS_QUALIFIER).value()),
					ocl::ADDRESS_QUALIFIER.at(kernel.get_arg_info_integral<cl_kernel_arg_address_qualifier>(arg_index, CL_KERNEL_ARG_ADDRESS_QUALIFIER).value()),
					kernel.get_arg_info_string(arg_index, CL_KERNEL_ARG_TYPE_NAME).value(),
					ocl::TYPE_QUALIFIER.at(kernel.get_arg_info_integral<cl_kernel_arg_type_qualifier>(arg_index, CL_KERNEL_ARG_TYPE_QUALIFIER).value()),
					kernel.get_arg_info_string(arg_index, CL_KERNEL_ARG_NAME).value()
				);
#else
				std::cout << std::format("  ACCESS_QUALIFIER: {}\n", ocl::ACCESS_QUALIFIER.at(kernel.get_arg_info_integral<cl_kernel_arg_access_qualifier>(arg_index, CL_KERNEL_ARG_ACCESS_QUALIFIER).value()));
				std::cout << std::format("  ADDRESS_QUALIFIER: {}\n", ocl::ADDRESS_QUALIFIER.at(kernel.get_arg_info_integral<cl_kernel_arg_address_qualifier>(arg_index, CL_KERNEL_ARG_ADDRESS_QUALIFIER).value()));
				std::cout << std::format("  CL_KERNEL_ARG_TYPE_NAME: {}\n", kernel.get_arg_info_string(arg_index, CL_KERNEL_ARG_TYPE_NAME).value());
				std::cout << std::format("  TYPE_QUALIFIER: {}\n", ocl::TYPE_QUALIFIER.at(kernel.get_arg_info_integral<cl_kernel_arg_type_qualifier>(arg_index, CL_KERNEL_ARG_TYPE_QUALIFIER).value()));
				std::cout << std::format("  CL_KERNEL_ARG_NAME: {}\n", kernel.get_arg_info_string(arg_index, CL_KERNEL_ARG_NAME).value());
				puts("");
#endif
			}
			puts("");
		}
	}
	//if(sts.has_value()) return 0;
	//std::cout << std::format("status: {}\n", static_cast<int32_t>(sts.error()));
	return 1;
}