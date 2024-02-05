include_guard(GLOBAL)
function(file_user executable_target)
block()

  FetchContent_MakeAvailable(benchmark-files)

  add_executable("${executable_target}")
  target_sources("${executable_target}" PRIVATE "${executable_target}.cpp")
  target_sources("${executable_target}" PUBLIC "${executable_target}.h")
  target_link_libraries("${executable_target}" PUBLIC lib_dev_flat ${ARGN})

endblock()
endfunction()