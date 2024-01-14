#include "ocl.h"
#include "MemoryMappedFile.h"

void test_opencl() {
    ocl::platform::each([](uint64_t platform_idx, ocl::platform platform) {
        platform.log_info();
        platform.each_device([](uint64_t device_idx, ocl::device device) {
            device.log_info();
            auto max_workgroup_size{ device.get_info_integral<size_t>(CL_DEVICE_MAX_WORK_GROUP_SIZE).value() };
            auto context{ device.create_context().value() };
            std::cout << std::format("context.refcount: {}\n", context.get_reference_count().value());

            auto queue{ context.create_queue().value() };
            std::cout << std::format("queue.refcount: {}\n", queue.get_reference_count().value());

            size_t svmSpanSize{sizeof(uint32_t) * max_workgroup_size};
            auto svmSpan{std::span<uint32_t>(
                reinterpret_cast<uint32_t *>(clSVMAlloc(context.context, CL_MEM_READ_WRITE | CL_MEM_SVM_FINE_GRAIN_BUFFER, svmSpanSize, 0)), max_workgroup_size
            )};

            // auto mapEventA{queue.enqueueSVMMap<uint32_t>(false, CL_MAP_WRITE_INVALIDATE_REGION, svmSpan).value()};
            // std::cout << std::format("mapEventA.CL_EVENT_COMMAND_EXECUTION_STATUS : {}\n", mapEventA.get_info_integral<cl_int>(CL_EVENT_COMMAND_EXECUTION_STATUS).value());
            // mapEventA.wait().value();
            // std::cout << std::format("mapEventA.CL_EVENT_COMMAND_EXECUTION_STATUS : {}\n", mapEventA.get_info_integral<cl_int>(CL_EVENT_COMMAND_EXECUTION_STATUS).value());
            std::ranges::fill(svmSpan, 1);
            // auto unmapEventA{queue.enqueueSVMUnmap<uint32_t>(svmSpan).value()};
            // std::cout << std::format("unmapEventA.CL_EVENT_COMMAND_EXECUTION_STATUS : {}\n", unmapEventA.get_info_integral<cl_int>(CL_EVENT_COMMAND_EXECUTION_STATUS).value());
            // unmapEventA.wait().value();
            // std::cout << std::format("unmapEventA.CL_EVENT_COMMAND_EXECUTION_STATUS : {}\n", unmapEventA.get_info_integral<cl_int>(CL_EVENT_COMMAND_EXECUTION_STATUS).value());

            //auto buffer{ context.create_buffer(CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, svmSpan).value() };
            //std::cout << std::format("buffer.refcount: {}\n", buffer.get_reference_count().value());

            MemoryMappedFile source{ "../kernels/test_kernel_1.cl" };
            auto program{ context.create_program(source.get_span<char>()).value() };
            std::cout << std::format("program.refcount: {}\n", program.get_reference_count().value());

            program.build().value();
            std::cout << std::format("program.build_log: {}\n", program.get_build_info_string(CL_PROGRAM_BUILD_LOG).value());
            std::cout << std::format("program.kernels: {}\n", program.get_info_string(CL_PROGRAM_KERNEL_NAMES).value());

            auto kernel{ program.create_kernel("test_kernel_1").value() };
            std::cout << std::format("kerenl.CL_KERNEL_FUNCTION_NAME: {}\n", kernel.get_info_string(CL_KERNEL_FUNCTION_NAME).value());
            std::cout << std::format("kerenl.CL_KERNEL_ATTRIBUTES: {}\n", kernel.get_info_string(CL_KERNEL_ATTRIBUTES).value());
            std::cout << std::format("kerenl.refcount: {}\n", kernel.get_reference_count().value());
            std::cout << std::format("kerenl.num_args: {}\n", kernel.get_info_integral<cl_uint>(CL_KERNEL_NUM_ARGS).value());

            std::cout << std::format("kerenl[0].CL_KERNEL_ARG_NAME: {}\n", kernel.get_arg_info_string(0, CL_KERNEL_ARG_NAME).value());
            std::cout << std::format("kerenl[0].CL_KERNEL_ARG_TYPE_NAME: {}\n", kernel.get_arg_info_string(0, CL_KERNEL_ARG_TYPE_NAME).value());

            kernel.set_arg(0, svmSpan).value();
            auto kernel_event{ queue.enqueue<1>(kernel.kernel, { 0 }, { svmSpan.size() }, {max_workgroup_size}).value()};
            std::cout << std::format("kernel_event.refcount: {}\n", kernel_event.get_reference_count().value());
            std::cout << std::format("kernel_event.CL_EVENT_COMMAND_TYPE : {}\n", kernel_event.get_info_integral<cl_command_type>(CL_EVENT_COMMAND_TYPE).value());
            std::cout << std::format("kernel_event.CL_EVENT_COMMAND_EXECUTION_STATUS : {}\n", kernel_event.get_info_integral<cl_int>(CL_EVENT_COMMAND_EXECUTION_STATUS).value());

            {
                kernel_event.wait().value();
                std::cout << std::format("kernel_event.CL_EVENT_COMMAND_EXECUTION_STATUS : {}\n", kernel_event.get_info_integral<cl_int>(CL_EVENT_COMMAND_EXECUTION_STATUS).value());

                // auto mapEvent{queue.enqueueSVMMap<uint32_t>(false, CL_MAP_READ, svmSpan).value()};
                // std::cout << std::format("mapEvent.CL_EVENT_COMMAND_EXECUTION_STATUS : {}\n", mapEvent.get_info_integral<cl_int>(CL_EVENT_COMMAND_EXECUTION_STATUS).value());
                // queue.flush().value();
                // std::cout << std::format("mapEvent.CL_EVENT_COMMAND_EXECUTION_STATUS : {}\n", mapEvent.get_info_integral<cl_int>(CL_EVENT_COMMAND_EXECUTION_STATUS).value());
                // mapEvent.wait().value();
                // std::cout << std::format("mapEvent.CL_EVENT_COMMAND_EXECUTION_STATUS : {}\n", mapEvent.get_info_integral<cl_int>(CL_EVENT_COMMAND_EXECUTION_STATUS).value());

                each(svmSpan, [](uint64_t i, uint32_t n) {
                    if (i != n) std::cout << std::format("mismatch[{}] != {}\n", i, n);
                });
                // auto unmapEvent{queue.enqueueSVMUnmap<uint32_t>(svmSpan).value()};
                // std::cout << std::format("unmapEvent.CL_EVENT_COMMAND_EXECUTION_STATUS : {}\n", unmapEvent.get_info_integral<cl_int>(CL_EVENT_COMMAND_EXECUTION_STATUS).value());
                // queue.flush().value();
                // std::cout << std::format("unmapEvent.CL_EVENT_COMMAND_EXECUTION_STATUS : {}\n", unmapEvent.get_info_integral<cl_int>(CL_EVENT_COMMAND_EXECUTION_STATUS).value());
                // unmapEvent.wait().value();
                // std::cout << std::format("unmapEvent.CL_EVENT_COMMAND_EXECUTION_STATUS : {}\n", unmapEvent.get_info_integral<cl_int>(CL_EVENT_COMMAND_EXECUTION_STATUS).value());
            }
            // queue.finish().value();

        });
    });
}

int main() {
  test_opencl();
  return 0;
}