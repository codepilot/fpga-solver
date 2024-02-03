include_guard(GLOBAL)
block()
  set(BASE_NAME dev_flat)
  set(executable_target "make_${BASE_NAME}")
  message("executable_target ${executable_target}")

  add_executable(${executable_target})
  target_sources(${executable_target} PRIVATE ${executable_target}.cpp)
  target_link_libraries(${executable_target} PUBLIC interchange-files)

  FetchContent_MakeAvailable(device-file)

  set(DEVICE_SOURCE_FILE ${device-file_SOURCE_DIR}/xcvu3p.device)
  set(DEVICE_DEST_FILE ${device-file_BINARY_DIR}/xcvu3p.device)
  message("DEVICE_SOURCE_FILE = ${DEVICE_SOURCE_FILE}")
  message("DEVICE_DEST_FILE   = ${DEVICE_DEST_FILE}")
  message("PROJECT_BINARY_DIR = ${PROJECT_BINARY_DIR}")

  set(lib_target "lib_${BASE_NAME}")
  message("lib_target ${lib_target}")
  set(lib_target_cpp "${PROJECT_BINARY_DIR}/${lib_target}.cpp")
  set(lib_target_header "${PROJECT_BINARY_DIR}/${lib_target}.h")
  message("lib_target_cpp ${lib_target_cpp}")
  message("lib_target_header ${lib_target_header}")

  add_custom_command(
    OUTPUT
      "${DEVICE_DEST_FILE}"
      "${lib_target_cpp}"
      "${lib_target_header}"
    DEPENDS
      "$<TARGET_FILE:${executable_target}>"
      "${DEVICE_SOURCE_FILE}"
    COMMAND
      "$<TARGET_FILE:${executable_target}>"
      "${DEVICE_SOURCE_FILE}"
      "${DEVICE_DEST_FILE}"
    WORKING_DIRECTORY
      "${PROJECT_BINARY_DIR}"
  )

  add_library(${lib_target} STATIC)
  target_sources(${lib_target} PRIVATE ${lib_target_cpp})
  target_sources(${lib_target} PUBLIC ${lib_target_header})
  target_link_libraries(${lib_target} PUBLIC interchange-files)

endblock()
