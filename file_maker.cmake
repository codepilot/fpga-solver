include_guard(GLOBAL)
block()
  function(file_maker)
    block()
      set(executable_target "make_${ARGV0}")
      message("executable_target ${executable_target}")

      add_executable(${executable_target})
      target_sources(${executable_target} PRIVATE ${executable_target}.cpp)
      target_sources(${executable_target} PUBLIC ${executable_target}.h)
      target_link_libraries(${executable_target} PUBLIC lib_dev_flat)

      set(lib_target "lib_${ARGV0}")
      message("lib_target ${lib_target}")

      set(lib_target_bin "${PROJECT_BINARY_DIR}/${lib_target}.bin")
      set(lib_target_cpp "${PROJECT_BINARY_DIR}/${lib_target}.cpp")
      set(lib_target_header "${PROJECT_BINARY_DIR}/${lib_target}.h")

      message("lib_target_bin ${lib_target_bin}")
      message("lib_target_cpp ${lib_target_cpp}")
      message("lib_target_header ${lib_target_header}")

      add_library(${lib_target} STATIC)

      add_custom_command(
        OUTPUT
          "${lib_target_bin}"
          "${lib_target_cpp}"
          "${lib_target_header}"
        DEPENDS
          "$<TARGET_FILE:${executable_target}>"
        COMMAND
          "$<TARGET_FILE:${executable_target}>"
          "${lib_target_bin}"
          "${lib_target_cpp}"
          "${lib_target_header}"
        WORKING_DIRECTORY
          "${PROJECT_BINARY_DIR}"
      )
  
      target_sources(${lib_target} PRIVATE ${lib_target_cpp})
      target_sources(${lib_target} PUBLIC ${lib_target_header})
      target_link_libraries(${lib_target} PUBLIC lib_dev_flat)
    
    endblock()
  endfunction()
endblock()
