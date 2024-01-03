cmake_minimum_required(VERSION 3.10)
include(ExternalProject)
include(FetchContent)

project(fpga-solver LANGUAGES CXX)

set(CAPNP_LITE OFF)
set(WITH_ZLIB OFF)
add_subdirectory(capnproto)

set(ZLIB_INCLUDE_DIR "$<TARGET_PROPERTY:zlib,SOURCE_DIR>;$<TARGET_PROPERTY:zlib,BINARY_DIR>")
set(ZLIB_INCLUDE_DIRS "$<TARGET_PROPERTY:zlib,SOURCE_DIR>;$<TARGET_PROPERTY:zlib,BINARY_DIR>")

add_subdirectory(zlib)
target_include_directories(example PRIVATE ${ZLIB_INCLUDE_DIR})
target_include_directories(example64 PRIVATE ${ZLIB_INCLUDE_DIR})
target_include_directories(minigzip PRIVATE ${ZLIB_INCLUDE_DIR})
target_include_directories(minigzip64 PRIVATE ${ZLIB_INCLUDE_DIR})
set(ZLIB_LIBRARY zlibstatic)


add_subdirectory(OpenCL-Headers)
add_subdirectory(OpenCL-ICD-Loader)
add_subdirectory(OpenCL-CLHPP)
# add_dependencies(kj-gzip zlibstatic)
# target_link_libraries(kj-heavy-tests zlibstatic)
add_subdirectory(SPIRV-Headers)
add_subdirectory(SPIRV-Tools)
add_subdirectory(glslang)
# # message(${ZLIB_LIBRARY})
# message(${ZLIB_INCLUDE_DIR})
# set(PNG_BUILD_ZLIB ON)
# file(REAL_PATH zlib/RelWithDebInfo/zlibstatic.lib ZLIBSTATIC_LIB BASE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
# set(ZLIB_LIBRARIES ${ZLIBSTATIC_LIB})
# add_subdirectory(libpng)
# add_dependencies(png_shared zlibstatic)
# add_dependencies(png_static zlibstatic)
# # ExternalProject_Add(libclcxx GIT_REPOSITORY https://github.com/KhronosGroup/libclcxx.git GIT_TAG main GIT_PROGRESS true)

# configure_file("libpng/scripts/pnglibconf.h.prebuilt" "build/libpng/pnglibconf.h")

# add_executable(${PROJECT_NAME} WIN32 win-desktop.cpp)
# target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" ${PROJECT_BINARY_DIR}/interchange)
# target_include_directories(${PROJECT_NAME} PUBLIC OpenGL-Registry/api OpenGL-Registry/extensions EGL-Registry/api OpenCL-CLHPP/include OpenCL-Headers capnproto/c++/src $<TARGET_PROPERTY:png_shared,SOURCE_DIR> $<TARGET_PROPERTY:png_shared,BINARY_DIR>)
# target_include_directories(${PROJECT_NAME} PRIVATE ${ZLIB_INCLUDE_DIR})
# target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX)
# target_compile_definitions(${PROJECT_NAME} PRIVATE CL_HPP_ENABLE_EXCEPTIONS)
# target_compile_definitions(${PROJECT_NAME} PRIVATE CL_HPP_TARGET_OPENCL_VERSION=300)
# target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)
# target_compile_options(${PROJECT_NAME} PRIVATE $<$<BOOL:${MSVC}>:/arch:AVX2 /Zc:__cplusplus /MP /GR->)
# target_link_libraries(${PROJECT_NAME} PRIVATE capnp)
# target_link_libraries(${PROJECT_NAME} PRIVATE OpenCL)
# target_link_libraries(${PROJECT_NAME} PRIVATE png_static)
# target_link_libraries(${PROJECT_NAME} PRIVATE Opengl32.lib)
# target_link_libraries(${PROJECT_NAME} PRIVATE Kernel32.lib)
# target_link_libraries(${PROJECT_NAME} PRIVATE WindowsApp.lib)
# target_link_libraries(${PROJECT_NAME} PRIVATE Avrt.lib)

# add_executable(build-cache build-cache.cpp)
# target_include_directories(build-cache PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" ${PROJECT_BINARY_DIR}/interchange)
# target_include_directories(build-cache PRIVATE ${ZLIB_INCLUDE_DIR})
# target_include_directories(build-cache PUBLIC capnproto/c++/src)
# target_compile_definitions(build-cache PRIVATE NOMINMAX)
# target_compile_features(build-cache PUBLIC cxx_std_23)
# target_compile_options(build-cache PRIVATE $<$<BOOL:${MSVC}>:/arch:AVX2 /Zc:__cplusplus /MP /GR->)
# target_link_libraries(build-cache PRIVATE capnp)
# target_link_libraries(build-cache PRIVATE zlibstatic)
# target_link_libraries(build-cache PRIVATE Kernel32.lib)
# target_link_libraries(build-cache PRIVATE WindowsApp.lib)

# add_executable(canonize canonize.cpp)
# target_include_directories(canonize PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
# target_include_directories(canonize PRIVATE ${ZLIB_INCLUDE_DIR})
# target_include_directories(canonize PUBLIC capnproto/c++/src)
# target_compile_definitions(canonize PRIVATE NOMINMAX)
# target_compile_features(canonize PUBLIC cxx_std_23)
# target_compile_options(canonize PRIVATE $<$<BOOL:${MSVC}>:/arch:AVX2 /Zc:__cplusplus /MP /GR->)
# target_link_libraries(canonize PRIVATE capnp)
# target_link_libraries(canonize PRIVATE zlibstatic)
# target_link_libraries(canonize PRIVATE Kernel32.lib)
# target_link_libraries(canonize PRIVATE WindowsApp.lib)

add_executable(show_phys_nets show_phys_nets.cpp)
target_include_directories(show_phys_nets PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" ${PROJECT_BINARY_DIR}/interchange)
target_include_directories(show_phys_nets PRIVATE ${ZLIB_INCLUDE_DIR})
target_include_directories(show_phys_nets PUBLIC capnproto/c++/src)
target_compile_definitions(show_phys_nets PRIVATE NOMINMAX)
target_compile_features(show_phys_nets PUBLIC cxx_std_23)
target_compile_options(show_phys_nets PRIVATE -march=native -std=c++2b $<$<BOOL:${MSVC}>:/arch:AVX2 /Zc:__cplusplus /MP /GR->)
target_link_libraries(show_phys_nets PRIVATE capnp)
target_link_libraries(show_phys_nets PRIVATE zlibstatic)
if(MSVC)
  target_link_libraries(show_phys_nets PRIVATE Kernel32.lib)
  target_link_libraries(show_phys_nets PRIVATE WindowsApp.lib)
endif()

add_executable(make_pips make_pips.cpp)
target_include_directories(make_pips PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" ${PROJECT_BINARY_DIR}/interchange)
target_include_directories(make_pips PRIVATE ${ZLIB_INCLUDE_DIR})
target_include_directories(make_pips PUBLIC capnproto/c++/src)
target_compile_definitions(make_pips PRIVATE NOMINMAX)
target_compile_features(make_pips PUBLIC cxx_std_23)
target_compile_options(make_pips PRIVATE -march=native -std=c++2b $<$<BOOL:${MSVC}>:/arch:AVX2 /Zc:__cplusplus /MP /GR->)
target_link_libraries(make_pips PRIVATE capnp)
target_link_libraries(make_pips PRIVATE zlibstatic)
if(MSVC)
  target_link_libraries(make_pips PRIVATE Kernel32.lib)
  target_link_libraries(make_pips PRIVATE WindowsApp.lib)
endif()

add_executable(route_phys route_phys.cpp)
target_include_directories(route_phys PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" ${PROJECT_BINARY_DIR}/interchange)
target_include_directories(route_phys PRIVATE ${ZLIB_INCLUDE_DIR})
target_include_directories(route_phys PUBLIC capnproto/c++/src)
target_compile_definitions(route_phys PRIVATE NOMINMAX)
target_compile_features(route_phys PUBLIC cxx_std_23)
target_compile_options(route_phys PRIVATE -march=native -std=c++2b $<$<BOOL:${MSVC}>:/arch:AVX2 /Zc:__cplusplus /MP /GR->)
target_link_libraries(route_phys PRIVATE capnp)
target_link_libraries(route_phys PRIVATE zlibstatic)
if(MSVC)
  target_link_libraries(route_phys PRIVATE Kernel32.lib)
  target_link_libraries(route_phys PRIVATE WindowsApp.lib)
endif()

# file(GLOB_RECURSE GLSL_SOURCE_FILES
#     "shaders/*.frag"
#     "shaders/*.vert"
# )

# foreach(GLSL ${GLSL_SOURCE_FILES})
#   get_filename_component(FILE_NAME ${GLSL} NAME)
#   set(SPIRV "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}.spv")
#   add_custom_command(
#     OUTPUT ${SPIRV}
#     COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shaders/"
#     COMMAND $<TARGET_FILE:glslang-standalone> -G ${GLSL} -o ${SPIRV}
#     DEPENDS ${GLSL}
#     DEPENDS glslang-standalone)
#   list(APPEND SPIRV_BINARY_FILES ${SPIRV})
# endforeach(GLSL)

# FetchContent_Declare(device-file URL https://github.com/Xilinx/fpga24_routing_contest/releases/latest/download/xcvu3p.device DOWNLOAD_NO_EXTRACT true)
# FetchContent_Declare(benchmark-files URL https://github.com/Xilinx/fpga24_routing_contest/releases/latest/download/benchmarks.tar.gz DOWNLOAD_EXTRACT_TIMESTAMP true)
# FetchContent_MakeAvailable(device-file benchmark-files)

# file(GLOB_RECURSE BENCHMARK_FILES "${benchmark-files_SOURCE_DIR}/*.netlist" "${benchmark-files_SOURCE_DIR}/*.phys" "${device-file_SOURCE_DIR}/*.device")

# foreach(BENCHMARK_FILE ${BENCHMARK_FILES})
#   get_filename_component(FILE_NAME ${BENCHMARK_FILE} NAME)
#   set(UNZIPPED "${PROJECT_BINARY_DIR}/benchmarks/${FILE_NAME}.UNZIPPED")
#   set(CANON "${PROJECT_BINARY_DIR}/benchmarks/${FILE_NAME}")
#   add_custom_command(
#     OUTPUT  ${CANON}
#     COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/benchmarks/"
#     COMMAND $<TARGET_FILE:canonize> ${BENCHMARK_FILE} ${UNZIPPED} ${CANON}
#     DEPENDS ${BENCHMARK_FILE}
#     DEPENDS canonize)
#   list(APPEND PREPARED_BENCHMARK_FILES ${CANON})
# endforeach(BENCHMARK_FILE)




# add_custom_target(Shaders DEPENDS ${SPIRV_BINARY_FILES})
# add_custom_target(prepared-benchmark-files DEPENDS ${PREPARED_BENCHMARK_FILES})


# file(GLOB_RECURSE DEVICE_FILES "${PROJECT_BINARY_DIR}/benchmarks/*.device")

# foreach(DEVICE_FILE ${DEVICE_FILES})
#   get_filename_component(FILE_NAME ${DEVICE_FILE} NAME)
#   set(INDIRECT_CACHE_FILE "${PROJECT_BINARY_DIR}/cache/${FILE_NAME}.indirect.bin")
#   set(DIRECT_CACHE_FILE "${PROJECT_BINARY_DIR}/cache/${FILE_NAME}.direct.bin")
#   set(DIRECT_DATA_CACHE_FILE "${PROJECT_BINARY_DIR}/cache/${FILE_NAME}.direct.data.bin")

#   add_custom_command(
#     OUTPUT ${INDIRECT_CACHE_FILE} ${DIRECT_CACHE_FILE} ${DIRECT_DATA_CACHE_FILE}
#     COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/cache/"
#     COMMAND $<TARGET_FILE:build-cache> ${DEVICE_FILE} ${INDIRECT_CACHE_FILE} ${DIRECT_CACHE_FILE} ${DIRECT_DATA_CACHE_FILE}
#     DEPENDS $<TARGET_FILE:build-cache>
#     DEPENDS ${DEVICE_FILE}
#   )
#   list(APPEND CACHE_BINARY_FILES ${INDIRECT_CACHE_FILE})
#   list(APPEND CACHE_BINARY_FILES ${DIRECT_CACHE_FILE})
#   list(APPEND CACHE_BINARY_FILES ${DIRECT_DATA_CACHE_FILE})
# endforeach(DEVICE_FILE)

# add_custom_target(cache-files DEPENDS ${CACHE_BINARY_FILES})
# add_dependencies(cache-files prepared-benchmark-files)


# add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders/"
#     COMMAND ${CMAKE_COMMAND} -E copy_directory
#         "${PROJECT_BINARY_DIR}/shaders"
#         "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders")

file(GLOB_RECURSE CAPNP_SOURCE_FILES
    "fpga-interchange-schema/interchange/*.capnp"
)

add_custom_target(interchange-files)

foreach(CAPNP ${CAPNP_SOURCE_FILES})
  get_filename_component(FILE_NAME ${CAPNP} NAME)
  set(CAPNPH "${PROJECT_BINARY_DIR}/interchange/${FILE_NAME}.h")
  set(CAPNPCPP "${PROJECT_BINARY_DIR}/interchange/${FILE_NAME}.c++")
  set(CAPNP_SRC "${PROJECT_BINARY_DIR}/interchange/${FILE_NAME}")
  add_custom_command(
    TARGET interchange-files
    WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/interchange"
    COMMAND $<TARGET_FILE:capnp_tool> compile --import-path="${CMAKE_CURRENT_SOURCE_DIR}/capnproto-java/compiler/src/main/schema" --import-path="${CMAKE_CURRENT_SOURCE_DIR}/capnproto/c++/src" --output="$<TARGET_FILE:capnpc_cpp>" --src-prefix=${PROJECT_BINARY_DIR}/interchange ${CAPNP_SRC}
    # DEPENDS ${CAPNP_SRC}
    DEPENDS "${PROJECT_BINARY_DIR}/interchange"
    DEPENDS capnp_tool)
  list(APPEND CAPNPH_FILES ${CAPNPH})
  list(APPEND CAPNPH_FILES ${CAPNPCPP})
endforeach(CAPNP)

add_custom_command(TARGET interchange-files PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/interchange/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/fpga-interchange-schema/interchange"
        "${PROJECT_BINARY_DIR}/interchange")




# add_dependencies(${PROJECT_NAME} Shaders cache-files prepared-benchmark-files interchange-files)
# add_dependencies(build-cache interchange-files)
add_dependencies(show_phys_nets interchange-files)
add_dependencies(route_phys interchange-files)
