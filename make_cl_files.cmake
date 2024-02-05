include_guard(GLOBAL)
block()
  standard_executable(make_cl_files)
  add_dependencies(make_cl_files
    make_dev_flat
    # make_search_files
  )

  set(tt_count_offset ${PROJECT_BINARY_DIR}/tt_count_offset.bin)
  set(tt_body ${PROJECT_BINARY_DIR}/tt_body.bin)

  add_custom_command(TARGET make_cl_files POST_BUILD
    COMMAND $<TARGET_FILE:make_cl_files>
    BYPRODUCTS ${tt_count_offset} ${tt_body}
    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
  )

endblock()