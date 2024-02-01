cmake_minimum_required(VERSION 3.27.7)

project(fpga-solver LANGUAGES CXX)

set(USE_SPIRV off)

include(repos.cmake)

include(block_capnproto.cmake)

include(block_zlib.cmake)

if(USE_SPIRV)
  include(shaders.cmake)
endif()

include(standard_executable.cmake)
# set(ZLIB_INCLUDE_DIR "$<TARGET_PROPERTY:zlib,SOURCE_DIR>;$<TARGET_PROPERTY:zlib,BINARY_DIR>")
# set(ZLIB_INCLUDE_DIRS "$<TARGET_PROPERTY:zlib,SOURCE_DIR>;$<TARGET_PROPERTY:zlib,BINARY_DIR>")

# set(CMAKE_PREFIX_PATH C:/Users/root/Desktop/fpga-solver/build/_deps/opencl-sdk-external-build/external/OpenCL-ICD-Loader)
# set(CMAKE_MODULE_PATH C:/Users/root/Desktop/fpga-solver/build/_deps/opencl-sdk-external-build/OpenCL;C:/Users/root/Desktop/fpga-solver/build/_deps/opencl-sdk-external-build/share/cmake/OpenCLUtilsCpp)

# file(REAL_PATH _deps/zlib-external-build/RelWithDebInfo/zlibstatic.lib ZLIBSTATIC_LIB BASE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
# set(ZLIB_LIBRARIES ${ZLIBSTATIC_LIB})

# set(ZLIB_LIBRARY zlib)

# set(TCLAP_INCLUDE_PATH "$<TARGET_PROPERTY:tclap,SOURCE_DIR>;$<TARGET_PROPERTY:tclap,BINARY_DIR>")

# set(Stb_INCLUDE_PATH "$<TARGET_PROPERTY:stb,SOURCE_DIR>;$<TARGET_PROPERTY:stb,BINARY_DIR>")


# set(OPENCL_SDK_BUILD_UTILITY_LIBRARIES OFF)
# set(OPENCL_SDK_BUILD_SAMPLES OFF)
# set(OPENCL_SDK_BUILD_OPENGL_SAMPLES OFF)
# set(OPENCL_SDK_TEST_SAMPLES OFF)



if(MSVC)
  include(win-desktop.cmake)
  include(build-cache.cmake)
  include(canonize.cmake)
endif()

include(show_phys_nets.cmake)

include(make_pips.cmake)
include(make_search_files.cmake)
include(search_files.cmake)

include(make_cl_files.cmake)
include(make_cl_pip_files.cmake)
include(make_site_pin_to_node.cmake)
include(make_wire_idx_to_node_idx.cmake)
include(site_pin_to_node.cmake)
include(wire_idx_to_node_idx.cmake)
include(cl_files.cmake)
include(cl_pip_files.cmake)
include(route_phys.cmake)


include(block_OpenCL-Headers.cmake)
include(block_OpenCL.cmake)

include(opencl-test.cmake)

include(opencl-interface.cmake)

include(opencl-tile-router.cmake)
include(opencl-node-router.cmake)



include(prepare-device.cmake)
include(prepare-benchmark.cmake)
include(interchange-files.cmake)

include(route_benchmarks.cmake)
if(MSVC)
  include(dcp_benchmarks.cmake)
  include(route_status_benchmarks.cmake)
endif()

# # file(GLOB_RECURSE DEVICE_FILES "${PROJECT_BINARY_DIR}/benchmarks/*.device")

# # foreach(DEVICE_FILE ${DEVICE_FILES})
# #   get_filename_component(FILE_NAME ${DEVICE_FILE} NAME)
# #   set(INDIRECT_CACHE_FILE "${PROJECT_BINARY_DIR}/cache/${FILE_NAME}.indirect.bin")
# #   set(DIRECT_CACHE_FILE "${PROJECT_BINARY_DIR}/cache/${FILE_NAME}.direct.bin")
# #   set(DIRECT_DATA_CACHE_FILE "${PROJECT_BINARY_DIR}/cache/${FILE_NAME}.direct.data.bin")

# #   add_custom_command(
# #     OUTPUT ${INDIRECT_CACHE_FILE} ${DIRECT_CACHE_FILE} ${DIRECT_DATA_CACHE_FILE}
# #     COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/cache/"
# #     COMMAND $<TARGET_FILE:build-cache> ${DEVICE_FILE} ${INDIRECT_CACHE_FILE} ${DIRECT_CACHE_FILE} ${DIRECT_DATA_CACHE_FILE}
# #     DEPENDS $<TARGET_FILE:build-cache>
# #     DEPENDS ${DEVICE_FILE}
# #   )
# #   list(APPEND CACHE_BINARY_FILES ${INDIRECT_CACHE_FILE})
# #   list(APPEND CACHE_BINARY_FILES ${DIRECT_CACHE_FILE})
# #   list(APPEND CACHE_BINARY_FILES ${DIRECT_DATA_CACHE_FILE})
# # endforeach(DEVICE_FILE)

# # add_custom_target(cache-files DEPENDS ${CACHE_BINARY_FILES})
# # add_dependencies(cache-files prepared-benchmark-files)





# if(MSVC)
# add_dependencies(win-desktop Shaders interchange-files)
# endif()

# # add_dependencies(win-desktop Shaders interchange-files)
# # add_dependencies(win-desktop Shaders cache-files prepared-benchmark-files interchange-files)
# # add_dependencies(build-cache interchange-files)
# add_dependencies(show_phys_nets interchange-files)
# add_dependencies(route_phys interchange-files)
# add_dependencies(make_pips interchange-files)
# add_dependencies(make_search_files interchange-files)
# add_dependencies(make_cl_files interchange-files)
