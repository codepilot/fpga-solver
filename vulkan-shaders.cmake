include_guard(GLOBAL)
block()
  find_package(SPIRV-Headers)
  find_package(SPIRV-Tools)
  find_package(glslang)

  file(GLOB_RECURSE GLSL_SOURCE_FILES
      "vulkan-shaders/*.glsl"
  )

  foreach(GLSL_SOURCE_FILE ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL_SOURCE_FILE} NAME)
    set(SPIRV "${PROJECT_BINARY_DIR}/${FILE_NAME}.spv")
    add_custom_command(
      OUTPUT ${SPIRV}
      COMMAND
        $<TARGET_FILE:glslang-standalone>
        --target-env vulkan1.3
        ${GLSL_SOURCE_FILE}
        -e main
        -H
        $<$<CONFIG:Debug>:-D_DEBUG=1>
        $<$<CONFIG:Debug>:-gVS>
        $<$<CONFIG:RelWithDebInfo>:-gVS>
        -o ${SPIRV}
      DEPENDS ${GLSL_SOURCE_FILE}
      DEPENDS glslang-standalone)
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
  endforeach(GLSL_SOURCE_FILE)

  add_custom_target(vulkan-shaders DEPENDS ${SPIRV_BINARY_FILES})
endblock()
