include_guard(GLOBAL)
block()

  set(site_pin_to_node_bin ${PROJECT_BINARY_DIR}/site_pin_to_node.bin)

  message("site_pin_to_node_bin = ${site_pin_to_node_bin}")

  add_custom_command(
    OUTPUT ${site_pin_to_node_bin}
    COMMAND $<TARGET_FILE:make_site_pin_to_node>
  )

  add_custom_target(site_pin_to_node DEPENDS ${site_pin_to_node_bin})
  add_dependencies(site_pin_to_node search_files make_site_pin_to_node wire_idx_to_node_idx)
endblock()