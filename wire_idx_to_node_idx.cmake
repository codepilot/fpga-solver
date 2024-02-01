include_guard(GLOBAL)
block()

  set(wire_idx_to_node_idx_bin ${PROJECT_BINARY_DIR}/wire_idx_to_node_idx.bin)

  message("wire_idx_to_node_idx_bin = ${wire_idx_to_node_idx_bin}")

  add_custom_command(
    OUTPUT ${wire_idx_to_node_idx_bin}
    COMMAND $<TARGET_FILE:make_wire_idx_to_node_idx>
    DEPENDS search_files)

  add_custom_target(wire_idx_to_node_idx DEPENDS ${wire_idx_to_node_idx_bin})
endblock()