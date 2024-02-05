cmake_minimum_required(VERSION 3.27.9)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()


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
    file_maker("site_pin_to_wire" lib_inverse_wires)
    file_maker("site_pin_to_node" lib_wire_idx_to_node_idx lib_inverse_wires)
    file_maker("wire_idx_to_tile_idx_tile_wire_idx" lib_inverse_wires)

include(file_user.cmake)
  file_user("tile-based-router" lib_dev_tile_index lib_inverse_wires lib_site_pin_to_wire lib_site_pin_to_node lib_wire_idx_to_tile_idx_tile_wire_idx)