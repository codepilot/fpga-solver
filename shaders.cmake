include_guard(GLOBAL)
block()
  file(GLOB_RECURSE GLSL_SOURCE_FILES
      "shaders/*.frag"
      "shaders/*.vert"
  )

  find_package(SPIRV-Headers)
  find_package(SPIRV-Tools)
  find_package(glslang)

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

  add_custom_target(shaders DEPENDS ${SPIRV_BINARY_FILES})
  add_dependencies(glslang-standalone)
endblock()