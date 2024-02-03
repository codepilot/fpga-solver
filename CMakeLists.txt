cmake_minimum_required(VERSION 3.28.2)

project(fpga-solver LANGUAGES CXX)

include(repos.cmake)
  include(block_capnproto.cmake)
  include(block_zlib.cmake)

include(interchange-files.cmake)
  include(make_dev_flat.cmake)
    include(file_maker.cmake)
    file_maker("dev_tile_index")
      include(tile-based-router.cmake)
