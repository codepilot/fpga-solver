include_guard(GLOBAL)
block()

FetchContent_MakeAvailable(benchmark-files)
file(GLOB_RECURSE NETLIST_FILES "${benchmark-files_SOURCE_DIR}/*.netlist")

add_custom_target(route_benchmarks DEPENDS opencl-node-router)

foreach(NETLIST_FILE ${NETLIST_FILES})
  get_filename_component(BENCHMARK_FILE_NAME "${NETLIST_FILE}" NAME_WE)
  set(UNROUTED_PHYS_FILE "${benchmark-files_SOURCE_DIR}/${BENCHMARK_FILE_NAME}_unrouted.phys")
  set(ROUTED_PHYS_FILE "${benchmark-files_BINARY_DIR}/${BENCHMARK_FILE_NAME}.phys")
  set(LOG_FILE "${PROJECT_SOURCE_DIR}/routing-results/${BENCHMARK_FILE_NAME}.txt")
  message("route_benchmark: opencl-node-router ${UNROUTED_PHYS_FILE} ${ROUTED_PHYS_FILE}")

  add_custom_command(TARGET route_benchmarks POST_BUILD COMMAND $<TARGET_FILE:opencl-node-router> "${UNROUTED_PHYS_FILE}" "${ROUTED_PHYS_FILE}" > "${LOG_FILE}")
endforeach(NETLIST_FILE)

endblock()