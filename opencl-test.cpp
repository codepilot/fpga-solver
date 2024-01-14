#include "ocl.h"

void test_opencl() {
    ocl::each_platform([](uint64_t platform_idx, ocl::platform platform) {
        platform.log_info();
    });
}

int main() {
  test_opencl();
  return 0;
}