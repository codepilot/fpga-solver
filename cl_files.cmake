include_guard(GLOBAL)
block()

  set(tt_count_offset ${PROJECT_BINARY_DIR}/tt_count_offset.bin)
  set(tt_body ${PROJECT_BINARY_DIR}/tt_body.bin)

  list(APPEND cl_file_set ${tt_count_offset})
  list(APPEND cl_file_set ${tt_body})

  message("tt_count_offset = ${tt_count_offset}")
  message("tt_body = ${tt_body}")

  add_custom_command(
    OUTPUT ${cl_file_set}
    COMMAND $<TARGET_FILE:make_cl_files>
    DEPENDS search_files)

  message("cl_file_set = ${cl_file_set}")

  add_custom_target(cl_files DEPENDS ${cl_file_set})
endblock()