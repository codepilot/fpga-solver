#include "ocl.h"
#include "MemoryMappedFile.h"

void test_opencl() {
    ocl::platform::each([](uint64_t platform_idx, ocl::platform platform) {
        platform.log_info();
        platform.each_device([](uint64_t device_idx, ocl::device device) {
            device.log_info();
            auto c{ device.create_context().value() };
            std::cout << std::format("context.refcount: {}\n", c.get_reference_count().value());

            auto q{ c.create_command_queue().value() };
            std::cout << std::format("queue.refcount: {}\n", q.get_reference_count().value());

            auto b{ c.create_buffer(CL_MEM_READ_WRITE, 65536ull).value() };
            std::cout << std::format("buffer.refcount: {}\n", b.get_reference_count().value());

            MemoryMappedFile source{ "../kernels/test_kernel_1.cl" };
            auto p{ c.create_program(source.get_span<char>()).value() };
            std::cout << std::format("program.refcount: {}\n", p.get_reference_count().value());

            p.build().value();
            std::cout << std::format("program.build_log: {}\n", p.get_build_info_string(CL_PROGRAM_BUILD_LOG).value());
            std::cout << std::format("program.kernels: {}\n", p.get_info_string(CL_PROGRAM_KERNEL_NAMES).value());

        });
    });
}

int main() {
  test_opencl();
  return 0;
}