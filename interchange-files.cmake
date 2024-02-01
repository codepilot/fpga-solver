include_guard(GLOBAL)
block()

  FetchContent_MakeAvailable(fpga-interchange-schema)
  FetchContent_MakeAvailable(capnproto-java)

  # message("fpga-interchange-schema_SOURCE_DIR = ${fpga-interchange-schema_SOURCE_DIR}")
  # message("capnproto-java_SOURCE_DIR = ${capnproto-java_SOURCE_DIR}")

  file(GLOB_RECURSE CAPNP_SOURCE_FILES
      "${fpga-interchange-schema_SOURCE_DIR}/interchange/*.capnp"
  )

  add_custom_target(interchange-files)
  add_dependencies(interchange-files capnp_tool)

  foreach(CAPNP ${CAPNP_SOURCE_FILES})
    get_filename_component(FILE_NAME ${CAPNP} NAME)
    set(CAPNPH "${PROJECT_BINARY_DIR}/interchange/${FILE_NAME}.h")
    set(CAPNPCPP "${PROJECT_BINARY_DIR}/interchange/${FILE_NAME}.c++")
    set(CAPNP_SRC "${PROJECT_BINARY_DIR}/interchange/${FILE_NAME}")
    add_custom_command(
      TARGET interchange-files
      WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/interchange"
      COMMAND $<TARGET_FILE:capnp_tool> compile --import-path="${capnproto-java_SOURCE_DIR}/compiler/src/main/schema" --import-path="$<TARGET_PROPERTY:capnp,SOURCE_DIR>/.." --output="$<TARGET_FILE:capnpc_cpp>" --src-prefix=${PROJECT_BINARY_DIR}/interchange ${CAPNP_SRC}
      # DEPENDS ${CAPNP_SRC}
      DEPENDS "${PROJECT_BINARY_DIR}/interchange"
      DEPENDS capnp_tool)
    list(APPEND CAPNPH_FILES ${CAPNPH})
    list(APPEND CAPNPH_FILES ${CAPNPCPP})
  endforeach(CAPNP)

  add_custom_command(TARGET interchange-files PRE_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/interchange/"
      COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${fpga-interchange-schema_SOURCE_DIR}/interchange"
          "${PROJECT_BINARY_DIR}/interchange")

endblock()
