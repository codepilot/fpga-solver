cmake_minimum_required(VERSION 3.10)
project(fpga-solver LANGUAGES CXX)
add_executable(${PROJECT_NAME} WIN32 win-desktop.cpp)

include(ExternalProject)
target_compile_definitions(${PROJECT_NAME} PRIVATE NOMINMAX)
target_compile_definitions(${PROJECT_NAME} PRIVATE CL_HPP_ENABLE_EXCEPTIONS)
target_compile_definitions(${PROJECT_NAME} PRIVATE CL_HPP_TARGET_OPENCL_VERSION=300)

add_subdirectory(OpenCL-Headers)
add_subdirectory(OpenCL-ICD-Loader)
add_subdirectory(OpenCL-CLHPP)
add_subdirectory(capnproto)
add_subdirectory(SPIRV-Headers)
add_subdirectory(SPIRV-Tools)
add_subdirectory(glslang)
# ExternalProject_Add(libclcxx GIT_REPOSITORY https://github.com/KhronosGroup/libclcxx.git GIT_TAG main GIT_PROGRESS true)

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(${PROJECT_NAME} PUBLIC OpenGL-Registry/api OpenGL-Registry/extensions EGL-Registry/api OpenCL-CLHPP/include OpenCL-Headers capnproto/c++/src)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)
target_compile_options(${PROJECT_NAME} PRIVATE $<$<BOOL:${MSVC}>:/arch:AVX2 /Zc:__cplusplus /MP /GR->)

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

add_custom_target(Shaders DEPENDS ${SPIRV_BINARY_FILES})

add_dependencies(${PROJECT_NAME} Shaders)

target_link_libraries(${PROJECT_NAME} PRIVATE capnp)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenCL)
target_link_libraries(${PROJECT_NAME} PRIVATE Opengl32.lib)
target_link_libraries(${PROJECT_NAME} PRIVATE Kernel32.lib)
target_link_libraries(${PROJECT_NAME} PRIVATE WindowsApp.lib)
target_link_libraries(${PROJECT_NAME} PRIVATE Avrt.lib)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_BINARY_DIR}/shaders"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders")

