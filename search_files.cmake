include_guard(GLOBAL)
block()
  FetchContent_MakeAvailable(device-file)

  set(DEVICE_SOURCE_FILE ${device-file_SOURCE_DIR}/xcvu3p.device)
  set(DEVICE_DEST_FILE ${device-file_BINARY_DIR}/xcvu3p.device)
  message("DEVICE_SOURCE_FILE = ${DEVICE_SOURCE_FILE}")
  message("DEVICE_DEST_FILE   = ${DEVICE_DEST_FILE}")

  message("PROJECT_BINARY_DIR = ${PROJECT_BINARY_DIR}")

  add_custom_target(search_files DEPENDS make_search_files)

  add_custom_command(
    TARGET search_files
    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
    COMMAND $<TARGET_FILE:make_search_files> "${DEVICE_SOURCE_FILE}" "${DEVICE_DEST_FILE}"
  )

endblock()