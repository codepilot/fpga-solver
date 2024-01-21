#include "ocl.h"
#include "MemoryMappedFile.h"

void test_opencl() {
    puts("test_opencl()");
    ocl::platform::each([](uint64_t platform_idx, ocl::platform platform) {
        platform.log_info();
        platform.each_device([](uint64_t device_idx, ocl::device device) {
            device.log_info();
            auto max_workgroup_size{ device.get_info_integral<size_t>(CL_DEVICE_MAX_WORK_GROUP_SIZE).value() };
            auto context{ device.create_context().value() };
            std::cout << std::format("context.refcount: {}\n", context.get_reference_count().value());

            auto queues{ context.create_queues().value() };
            std::cout << std::format("queue.refcount: {}\n", queues[0].get_reference_count().value());

            size_t svmSpanSize{sizeof(uint32_t) * max_workgroup_size};
            auto svm{ context.alloc_svm<uint32_t>(device.supports_svm_fine_grain_buffer().value() ? (CL_MEM_READ_WRITE | CL_MEM_SVM_FINE_GRAIN_BUFFER) : CL_MEM_READ_WRITE, svmSpanSize).value()};

            std::ranges::fill(svm, 1);

            MemoryMappedFile source{ "../kernels/test_kernel_1.cl" };
            auto program{ context.create_program(source.get_span<char>()).value() };
            std::cout << std::format("program.refcount: {}\n", program.get_reference_count().value());

            program.build().value();
            // std::cout << std::format("program.build_log: {}\n", program.get_build_info_string(CL_PROGRAM_BUILD_LOG).value());
            std::cout << std::format("program.kernels: {}\n", program.get_info_string(CL_PROGRAM_KERNEL_NAMES).value());

            auto kernel{ program.create_kernel("test_kernel_1").value() };
            std::cout << std::format("kerenl.CL_KERNEL_FUNCTION_NAME: {}\n", kernel.get_info_string(CL_KERNEL_FUNCTION_NAME).value());
            std::cout << std::format("kerenl.CL_KERNEL_ATTRIBUTES: {}\n", kernel.get_info_string(CL_KERNEL_ATTRIBUTES).value());
            std::cout << std::format("kerenl.refcount: {}\n", kernel.get_reference_count().value());
            std::cout << std::format("kerenl.num_args: {}\n", kernel.get_info_integral<cl_uint>(CL_KERNEL_NUM_ARGS).value());

            std::cout << std::format("kerenl[0].CL_KERNEL_ARG_NAME: {}\n", kernel.get_arg_info_string(0, CL_KERNEL_ARG_NAME).value());
            std::cout << std::format("kerenl[0].CL_KERNEL_ARG_TYPE_NAME: {}\n", kernel.get_arg_info_string(0, CL_KERNEL_ARG_TYPE_NAME).value());

            kernel.set_arg(0, svm).value();
            auto kernel_event{ queues[0].enqueue<1>(kernel.kernel, {0}, {svm.size()}, {max_workgroup_size}).value()};
            std::cout << std::format("kernel_event.refcount: {}\n", kernel_event.get_reference_count().value());
            std::cout << std::format("kernel_event.CL_EVENT_COMMAND_TYPE : {}\n", kernel_event.get_info_integral<cl_command_type>(CL_EVENT_COMMAND_TYPE).value());
            std::cout << std::format("kernel_event.CL_EVENT_COMMAND_EXECUTION_STATUS : {}\n", kernel_event.get_info_integral<cl_int>(CL_EVENT_COMMAND_EXECUTION_STATUS).value());

            kernel_event.wait().value();
            std::cout << std::format("kernel_event.CL_EVENT_COMMAND_EXECUTION_STATUS : {}\n", kernel_event.get_info_integral<cl_int>(CL_EVENT_COMMAND_EXECUTION_STATUS).value());

            each(svm, [](uint64_t i, uint32_t n) {
                if (i != n) std::cout << std::format("mismatch[{}] != {}\n", i, n);
            });

        });
    });
}

int main() {
  test_opencl();
  return 0;
}