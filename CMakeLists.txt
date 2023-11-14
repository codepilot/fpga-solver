cmake_minimum_required(VERSION 3.10)
project(fpga-solver)
add_executable(fpga-solver WIN32 win-desktop.cpp)
target_include_directories(fpga-solver PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(fpga-solver PUBLIC OpenGL-Registry/api OpenGL-Registry/extensions EGL-Registry/api OpenCL-CLHPP/include OpenCL-Headers capnproto/c++/src)
target_compile_features(fpga-solver PUBLIC cxx_std_23)
target_link_directories(fpga-solver PUBLIC build/OpenCL-ICD-Loader/$(ConfigurationName) build/capnproto/c++/src/capnp/$(ConfigurationName) build/capnproto/c++/src/kj/$(ConfigurationName))
target_compile_options(fpga-solver PRIVATE $<$<BOOL:${MSVC}>:/arch:AVX2 /Zc:__cplusplus /MP /GR->)
add_subdirectory(OpenCL-Headers)
add_subdirectory(OpenCL-ICD-Loader)
add_subdirectory(OpenCL-CLHPP)
add_subdirectory(capnproto)
add_subdirectory(SPIRV-Headers)
add_subdirectory(SPIRV-Tools)
add_subdirectory(glslang)
set(GLSL_VALIDATOR "${PROJECT_BINARY_DIR}/glslang/StandAlone/$(ConfigurationName)/glslangValidator.exe")
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
    COMMAND ${GLSL_VALIDATOR} -G ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL}
    DEPENDS ${GLSL_VALIDATOR})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    Shaders 
    DEPENDS ${SPIRV_BINARY_FILES}
    )

add_dependencies(fpga-solver Shaders)
add_dependencies(Shaders glslang)

add_custom_command(TARGET fpga-solver POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:fpga-solver>/shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_BINARY_DIR}/shaders"
        "$<TARGET_FILE_DIR:fpga-solver>/shaders"
        )

