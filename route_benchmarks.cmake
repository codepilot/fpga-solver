include_guard(GLOBAL)
block()

FetchContent_MakeAvailable(benchmark-files)
file(GLOB_RECURSE BENCHMARK_FILES "${benchmark-files_SOURCE_DIR}/*.phys")

add_custom_target(route_benchmarks DEPENDS opencl-tile-router)

foreach(BENCHMARK_FILE ${BENCHMARK_FILES})
  add_custom_command(TARGET route_benchmarks POST_BUILD COMMAND $<TARGET_FILE:opencl-tile-router> ${BENCHMARK_FILE})
endforeach(BENCHMARK_FILE)

endblock()