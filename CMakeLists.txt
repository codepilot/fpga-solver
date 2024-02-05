cmake_minimum_required(VERSION 3.27.9)

project(fpga-solver LANGUAGES CXX)

include(repos.cmake)
  include(block_capnproto.cmake)
  include(block_zlib.cmake)

include(interchange-files.cmake)
  include(make_dev_flat.cmake)
    include(file_maker.cmake)
    file_maker("dev_tile_index")
    file_maker("inverse_wires")
    file_maker("wire_idx_to_node_idx")
    file_maker("site_pin_to_wire")
      target_link_libraries(make_site_pin_to_wire PUBLIC lib_inverse_wires)
      target_link_libraries(lib_site_pin_to_wire PUBLIC lib_inverse_wires)
    file_maker("site_pin_to_node")
      target_link_libraries(make_site_pin_to_node PUBLIC lib_wire_idx_to_node_idx lib_inverse_wires)
      target_link_libraries(lib_site_pin_to_node PUBLIC lib_wire_idx_to_node_idx lib_inverse_wires)


include(tile-based-router.cmake)
