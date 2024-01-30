include_guard(GLOBAL)
block()

FetchContent_MakeAvailable(benchmark-files)
file(GLOB_RECURSE NETLIST_FILES "${benchmark-files_SOURCE_DIR}/*.netlist")

add_custom_target(dcp_benchmarks DEPENDS route_benchmarks)

foreach(NETLIST_FILE ${NETLIST_FILES})
  get_filename_component(BENCHMARK_FILE_NAME "${NETLIST_FILE}" NAME_WE)
  if("vtr_mcml" STREQUAL BENCHMARK_FILE_NAME)
  else()
    set(UNROUTED_CMD_FILE "${benchmark-files_BINARY_DIR}/${BENCHMARK_FILE_NAME}_unrouted.dcp.cmd")
    set(CMD_FILE "${benchmark-files_BINARY_DIR}/${BENCHMARK_FILE_NAME}.dcp.cmd")
    set(UNROUTED_PHYS_FILE "${benchmark-files_SOURCE_DIR}/${BENCHMARK_FILE_NAME}_unrouted.phys")
    set(ROUTED_PHYS_FILE "${benchmark-files_BINARY_DIR}/${BENCHMARK_FILE_NAME}.phys")
    set(DCP_FILE "${benchmark-files_BINARY_DIR}/${BENCHMARK_FILE_NAME}.dcp")
    set(XDC_FILE "${benchmark-files_BINARY_DIR}/${BENCHMARK_FILE_NAME}.xdc")
    set(LOG_FILE "${PROJECT_SOURCE_DIR}/routing-results/${BENCHMARK_FILE_NAME}.txt")
    message("dcp_benchmarks: ${NETLIST_FILE} ${ROUTED_PHYS_FILE} ${DCP_FILE} ${XDC_FILE}")

    file(TOUCH "${XDC_FILE}")
    file(WRITE "${CMD_FILE}" "rapidwright.bat PhysicalNetlistToDcp \"${NETLIST_FILE}\" \"${ROUTED_PHYS_FILE}\" \"${XDC_FILE}\" \"${DCP_FILE}\"")
    file(WRITE "${UNROUTED_CMD_FILE}" "rapidwright.bat PhysicalNetlistToDcp \"${NETLIST_FILE}\" \"${UNROUTED_PHYS_FILE}\" \"${XDC_FILE}\" \"${DCP_FILE}\"")
    add_custom_command(TARGET dcp_benchmarks POST_BUILD COMMAND rapidwright.bat PhysicalNetlistToDcp "${NETLIST_FILE}" "${ROUTED_PHYS_FILE}" "${XDC_FILE}" "${DCP_FILE}" >> "${LOG_FILE}")
  endif()
endforeach(NETLIST_FILE)

endblock()