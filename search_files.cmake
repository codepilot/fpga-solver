include_guard(GLOBAL)
block()
  FetchContent_MakeAvailable(device-file)

  set(DEVICE_SOURCE_FILE ${device-file_SOURCE_DIR}/xcvu3p.device)
  set(DEVICE_DEST_FILE ${device-file_SOURCE_DIR}/xcvu3p.device.temp)
  message("DEVICE_SOURCE_FILE = ${DEVICE_SOURCE_FILE}")
  message("DEVICE_DEST_FILE   = ${DEVICE_DEST_FILE}")

  message("PROJECT_BINARY_DIR = ${PROJECT_BINARY_DIR}")

  list(APPEND search_file_set ${DEVICE_DEST_FILE})
  list(APPEND search_file_set ${PROJECT_BINARY_DIR}/sorted_wire_tile_wire.bin)
  list(APPEND search_file_set ${PROJECT_BINARY_DIR}/sorted_wire_tile_node.bin)
  list(APPEND search_file_set ${PROJECT_BINARY_DIR}/sorted_node_tile_pip.bin)
  list(APPEND search_file_set ${PROJECT_BINARY_DIR}/sorted_tile_pip_node.bin)
  list(APPEND search_file_set ${PROJECT_BINARY_DIR}/sorted_site_pin_wires.bin)
  list(APPEND search_file_set ${PROJECT_BINARY_DIR}/sorted_site_pin_nodes.bin)
  list(APPEND search_file_set ${PROJECT_BINARY_DIR}/search_tile_tile_pip.bin)

  add_custom_command(
    OUTPUT ${search_file_set}
    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
    COMMAND $<TARGET_FILE:make_search_files>
    DEPENDS ${device-file}
  )


  add_custom_target(search_files DEPENDS ${search_file_set})

endblock()