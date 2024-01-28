include_guard(GLOBAL)
block()

  set(pip_count_offset ${PROJECT_BINARY_DIR}/pip_count_offset.bin)
  set(pip_tile_body ${PROJECT_BINARY_DIR}/pip_tile_body.bin)
  set(pip_body ${PROJECT_BINARY_DIR}/pip_body.bin)
  set(inverse_wires ${PROJECT_BINARY_DIR}/Inverse_Wires.bin)

  list(APPEND cl_pip_file_set ${pip_count_offset})
  list(APPEND cl_pip_file_set ${pip_tile_body})
  list(APPEND cl_pip_file_set ${pip_body})
  list(APPEND cl_pip_file_set ${inverse_wires})

  message("pip_count_offset = ${pip_count_offset}")
  message("pip_tile_body = ${pip_tile_body}")
  message("pip_body = ${pip_body}")
  message(("inverse_wires = ${inverse_wires}"))

  add_custom_command(
    OUTPUT ${cl_pip_file_set}
    COMMAND $<TARGET_FILE:make_cl_pip_files>
    DEPENDS search_files)

  message("cl_pip_file_set = ${cl_pip_file_set}")

  add_custom_target(cl_pip_files DEPENDS ${cl_pip_file_set})
endblock()