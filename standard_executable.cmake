include_guard(GLOBAL)
block()
  function(standard_executable)
    block()
      FetchContent_MakeAvailable(OpenGL-Registry)
      FetchContent_MakeAvailable(EGL-Registry)

      # message("opengl-registry_SOURCE_DIR = ${opengl-registry_SOURCE_DIR}")
      # message("egl-registry_SOURCE_DIR = ${egl-registry_SOURCE_DIR}")
      add_executable(${ARGV0} ${ARGV1} ${ARGV0}.cpp)
      target_include_directories(${ARGV0} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" ${PROJECT_BINARY_DIR}/interchange)
      target_include_directories(${ARGV0} PUBLIC $<TARGET_PROPERTY:zlib,SOURCE_DIR>)
      target_include_directories(${ARGV0} PUBLIC $<TARGET_PROPERTY:zlib,BINARY_DIR>)
      target_include_directories(${ARGV0} PUBLIC $<TARGET_PROPERTY:capnp,SOURCE_DIR>/..)
      target_include_directories(${ARGV0} PUBLIC ${opengl-registry_SOURCE_DIR}/api)
      target_include_directories(${ARGV0} PUBLIC ${opengl-registry_SOURCE_DIR}/extensions)
      target_include_directories(${ARGV0} PUBLIC ${egl-registry_SOURCE_DIR}/api)
      target_include_directories(${ARGV0} PUBLIC ${egl-registry_SOURCE_DIR}/extensions)
      target_include_directories(${ARGV0} PUBLIC $<TARGET_PROPERTY:Headers,SOURCE_DIR>)

      target_compile_definitions(${ARGV0} PRIVATE NOMINMAX)
      target_compile_features(${ARGV0} PUBLIC cxx_std_23)
      if(MSVC)
        target_compile_options(${ARGV0} PRIVATE /arch:AVX2 /Zc:__cplusplus /MP /GR-)
      else()
        target_compile_options(${ARGV0} PRIVATE -march=native -std=c++2b -ltbb)
        target_link_libraries(${ARGV0} PRIVATE tbb)
      endif()
      target_link_libraries(${ARGV0} PRIVATE capnproto::capnp)
      target_link_libraries(${ARGV0} PRIVATE ZLIB::zlibstatic)
      target_link_libraries(${ARGV0} PRIVATE OpenCL::OpenCL)

      if(MSVC)
        target_link_libraries(${ARGV0} PRIVATE Kernel32.lib)
        target_link_libraries(${ARGV0} PRIVATE WindowsApp.lib)
        target_link_libraries(${ARGV0} PRIVATE Avrt.lib)
        target_link_libraries(${ARGV0} PRIVATE Opengl32.lib)
      endif()

      add_dependencies(${ARGV0} interchange-files)
    endblock()
  endfunction()
endblock()
