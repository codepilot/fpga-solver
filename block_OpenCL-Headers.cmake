include_guard(GLOBAL)
block()
  set(OPENCL_HEADERS_BUILD_TESTING off)
  set(OPENCL_HEADERS_BUILD_CXX_TESTS off)
  find_package(OpenCL-Headers)
endblock()
