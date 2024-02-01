include_guard(GLOBAL)
block()

FetchContent_MakeAvailable(benchmark-files)
file(GLOB_RECURSE NETLIST_FILES "${benchmark-files_SOURCE_DIR}/*.netlist")

add_custom_target(route_status_benchmarks)
add_dependencies((route_status_benchmarks dcp_benchmarks)

set(ALL_ROUTE_STATUS "${benchmark-files_BINARY_DIR}/ALL.route_status.cmd")
file(WRITE "${ALL_ROUTE_STATUS}" "")

foreach(NETLIST_FILE ${NETLIST_FILES})
  get_filename_component(BENCHMARK_FILE_NAME "${NETLIST_FILE}" NAME_WE)
  set(CMD_FILE "${benchmark-files_BINARY_DIR}/${BENCHMARK_FILE_NAME}.route_status.cmd")
  set(TCL_FILE "${benchmark-files_BINARY_DIR}/${BENCHMARK_FILE_NAME}.tcl")
  set(LOG_FILE "${PROJECT_SOURCE_DIR}/routing-results/${BENCHMARK_FILE_NAME}.txt")
  set(DCP_FILE "${benchmark-files_BINARY_DIR}/${BENCHMARK_FILE_NAME}.dcp")
  message("route_status_benchmarks: ${BENCHMARK_FILE_NAME} ${TCL_FILE} ${LOG_FILE} ${DCP_FILE}")

  file(WRITE "${TCL_FILE}" "report_route_status -append -file ${LOG_FILE}\nexit\n")
  SET(CMD_FILE_CONTENTS "vivado.bat -mode batch -source \"${TCL_FILE}\" \"${DCP_FILE}\"")
  file(WRITE "${CMD_FILE}" "${CMD_FILE_CONTENTS}")
  file(APPEND "${ALL_ROUTE_STATUS}" "start \"route_status ${BENCHMARK_FILE_NAME}\" /MIN /LOW ${CMD_FILE_CONTENTS}\n")
  add_custom_command(TARGET route_status_benchmarks POST_BUILD COMMAND vivado.bat -mode batch -source "${TCL_FILE}" "${DCP_FILE}")
endforeach(NETLIST_FILE)

endblock()