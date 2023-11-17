cmake_minimum_required(VERSION 3.10)
include(ExternalProject)
include(FetchContent)

project(fpga-solver LANGUAGES CXX)

add_subdirectory(zlib)
# set(ZLIB_LIBRARY zlibstatic)
set(ZLIB_INCLUDE_DIR "$<TARGET_PROPERTY:zlib,SOURCE_DIR>;$<TARGET_PROPERTY:zlib,BINARY_DIR>")

add_subdirectory(OpenCL-Headers)
add_subdirectory(OpenCL-ICD-Loader)
add_subdirectory(OpenCL-CLHPP)
add_subdirectory(capnproto)
# add_dependencies(kj-gzip zlibstatic)
add_subdirectory(SPIRV-Headers)
add_subdirectory(SPIRV-Tools)
add_subdirectory(glslang)
# ExternalProject_Add(libclcxx GIT_REPOSITORY https://github.com/KhronosGroup/libclcxx.git GIT_TAG main GIT_PROGRESS true)

add_executable(${PROJECT_NAME} WIN32 win-desktop.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(${PROJECT_NAME} PUBLIC OpenGL-Registry/api OpenGL-Registry/extensions EGL-Registry/api OpenCL-CLHPP/include OpenCL-Headers capnproto/c++/src)
target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX)
target_compile_definitions(${PROJECT_NAME} PRIVATE CL_HPP_ENABLE_EXCEPTIONS)
target_compile_definitions(${PROJECT_NAME} PRIVATE CL_HPP_TARGET_OPENCL_VERSION=300)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)
target_compile_options(${PROJECT_NAME} PRIVATE $<$<BOOL:${MSVC}>:/arch:AVX2 /Zc:__cplusplus /MP /GR->)
target_link_libraries(${PROJECT_NAME} PRIVATE capnp)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenCL)
target_link_libraries(${PROJECT_NAME} PRIVATE Opengl32.lib)
target_link_libraries(${PROJECT_NAME} PRIVATE Kernel32.lib)
target_link_libraries(${PROJECT_NAME} PRIVATE WindowsApp.lib)
target_link_libraries(${PROJECT_NAME} PRIVATE Avrt.lib)

add_executable(build-cache build-cache.cpp)
target_include_directories(build-cache PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(build-cache PRIVATE ${ZLIB_INCLUDE_DIR})
target_include_directories(build-cache PUBLIC capnproto/c++/src)
target_compile_definitions(build-cache PRIVATE NOMINMAX)
target_compile_features(build-cache PUBLIC cxx_std_23)
target_compile_options(build-cache PRIVATE $<$<BOOL:${MSVC}>:/arch:AVX2 /Zc:__cplusplus /MP /GR->)
target_link_libraries(build-cache PRIVATE capnp)
target_link_libraries(build-cache PRIVATE zlibstatic)
target_link_libraries(build-cache PRIVATE Kernel32.lib)
target_link_libraries(build-cache PRIVATE WindowsApp.lib)

add_executable(canonize canonize.cpp)
target_include_directories(canonize PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(canonize PRIVATE ${ZLIB_INCLUDE_DIR})
target_include_directories(canonize PUBLIC capnproto/c++/src)
target_compile_definitions(canonize PRIVATE NOMINMAX)
target_compile_features(canonize PUBLIC cxx_std_23)
target_compile_options(canonize PRIVATE $<$<BOOL:${MSVC}>:/arch:AVX2 /Zc:__cplusplus /MP /GR->)
target_link_libraries(canonize PRIVATE capnp)
target_link_libraries(canonize PRIVATE zlibstatic)
target_link_libraries(canonize PRIVATE Kernel32.lib)
target_link_libraries(canonize PRIVATE WindowsApp.lib)


file(GLOB_RECURSE GLSL_SOURCE_FILES
    "shaders/*.frag"
    "shaders/*.vert"
)

foreach(GLSL ${GLSL_SOURCE_FILES})
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}.spv")
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shaders/"
    COMMAND $<TARGET_FILE:glslang-standalone> -G ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL}
    DEPENDS glslang-standalone)
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

FetchContent_Declare(device-file URL https://github.com/Xilinx/fpga24_routing_contest/releases/latest/download/xcvu3p.device DOWNLOAD_NO_EXTRACT true)
FetchContent_Declare(benchmark-files URL https://github.com/Xilinx/fpga24_routing_contest/releases/latest/download/benchmarks.tar.gz DOWNLOAD_EXTRACT_TIMESTAMP true)
FetchContent_MakeAvailable(device-file benchmark-files)

file(GLOB_RECURSE BENCHMARK_FILES "${benchmark-files_SOURCE_DIR}/*.netlist" "${benchmark-files_SOURCE_DIR}/*.phys" "${device-file_SOURCE_DIR}/*.device")

foreach(BENCHMARK_FILE ${BENCHMARK_FILES})
  get_filename_component(FILE_NAME ${BENCHMARK_FILE} NAME)
  set(UNZIPPED "${PROJECT_BINARY_DIR}/benchmarks/${FILE_NAME}.UNZIPPED")
  set(CANON "${PROJECT_BINARY_DIR}/benchmarks/${FILE_NAME}.CANON")
  add_custom_command(
    OUTPUT ${UNZIPPED} ${CANON}
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/benchmarks/"
    COMMAND $<TARGET_FILE:canonize> ${BENCHMARK_FILE} ${UNZIPPED} ${CANON}
    DEPENDS ${BENCHMARK_FILE}
    DEPENDS canonize)
  list(APPEND PREPARED_BENCHMARK_FILES ${UNZIPPED})
  list(APPEND PREPARED_BENCHMARK_FILES ${CANON})
endforeach(BENCHMARK_FILE)



set(CACHE_FILE "${PROJECT_BINARY_DIR}/cache/direct.bin")
set(DEVICE_UNZIPPED_FILE "${PROJECT_BINARY_DIR}/cache/unzipped.device")
set(DEVICE_UNZIPPED_CANON_FILE "${PROJECT_BINARY_DIR}/cache/unzipped.canon.device")
add_custom_command(
  OUTPUT ${DEVICE_UNZIPPED_FILE} ${DEVICE_UNZIPPED_CANON_FILE} ${CACHE_FILE}
  COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/cache/"
  COMMAND $<TARGET_FILE:build-cache> ${device-file_SOURCE_DIR}/xcvu3p.device ${DEVICE_UNZIPPED_FILE} ${DEVICE_UNZIPPED_CANON_FILE} ${CACHE_FILE} 
  DEPENDS $<TARGET_FILE:build-cache>
  DEPENDS ${device-file_SOURCE_DIR}/xcvu3p.device
)
list(APPEND CACHE_BINARY_FILES ${DEVICE_UNZIPPED_FILE})
list(APPEND CACHE_BINARY_FILES ${DEVICE_UNZIPPED_CANON_FILE})
list(APPEND CACHE_BINARY_FILES ${CACHE_FILE})

add_custom_target(Shaders DEPENDS ${SPIRV_BINARY_FILES})
add_custom_target(cache-files DEPENDS ${CACHE_BINARY_FILES})
add_custom_target(prepared-benchmark-files DEPENDS ${PREPARED_BENCHMARK_FILES})

add_dependencies(${PROJECT_NAME} Shaders cache-files prepared-benchmark-files)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_BINARY_DIR}/shaders"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders")

