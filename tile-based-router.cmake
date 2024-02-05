include_guard(GLOBAL)
block()

  FetchContent_MakeAvailable(benchmark-files)

  add_executable(tile-based-router)
  target_sources(tile-based-router PRIVATE tile-based-router.cpp)
  target_sources(tile-based-router PUBLIC tile-based-router.h)
  target_link_libraries(tile-based-router PUBLIC lib_dev_tile_index)
  target_link_libraries(tile-based-router PUBLIC lib_inverse_wires)
  target_link_libraries(tile-based-router PUBLIC lib_site_pin_to_wire)
  target_link_libraries(tile-based-router PUBLIC lib_site_pin_to_node)
endblock()