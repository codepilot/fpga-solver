#include "ocl.h"

void test_opencl() {
    ocl::platform::each([](uint64_t platform_idx, ocl::platform platform) {
        platform.log_info();
        platform.each_device([](uint64_t device_idx, ocl::device device) {
            device.log_info();
            auto c{ device.create_context().value() };
            std::cout << std::format("context.refcount: {}\n", c.get_reference_count().value());
        });
    });
}

int main() {
  test_opencl();
  return 0;
}