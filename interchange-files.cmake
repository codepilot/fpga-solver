include_guard(GLOBAL)
block()

  FetchContent_MakeAvailable(fpga-interchange-schema)
  FetchContent_MakeAvailable(capnproto-java)

  file(GLOB_RECURSE CAPNP_SOURCE_FILES
      "${fpga-interchange-schema_SOURCE_DIR}/interchange/*.capnp"
  )

  add_library(interchange-files STATIC)
  target_link_libraries(interchange-files PUBLIC CapnProto::capnp)
  target_link_libraries(interchange-files PUBLIC ZLIB::zlibstatic)
  target_include_directories(interchange-files PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
  target_include_directories(interchange-files PUBLIC ${PROJECT_BINARY_DIR})
  target_include_directories(interchange-files PUBLIC "${fpga-interchange-schema_SOURCE_DIR}/interchange")
  target_include_directories(interchange-files PUBLIC $<TARGET_PROPERTY:ZLIB::zlibstatic,SOURCE_DIR>)
  target_include_directories(interchange-files PUBLIC $<TARGET_PROPERTY:ZLIB::zlibstatic,BINARY_DIR>)

  target_compile_definitions(interchange-files PUBLIC NOMINMAX)
  target_compile_features(interchange-files PUBLIC cxx_std_23)
  if(WIN32)
    target_link_libraries(interchange-files PUBLIC Kernel32.lib)
    target_link_libraries(interchange-files PUBLIC WindowsApp.lib)
    target_link_libraries(interchange-files PUBLIC Avrt.lib)
  endif()
  if(LINUX)
    target_compile_options(interchange-files PUBLIC -march=native -std=c++2b -ltbb)
    target_link_libraries(interchange-files PUBLIC tbb)
  endif()
  if(MSVC)
    target_compile_options(interchange-files PUBLIC /arch:AVX2 /Zc:__cplusplus /MP /GR- /constexpr:steps1073741824 /bigobj)
  endif()

  foreach(CAPNP ${CAPNP_SOURCE_FILES})
    get_filename_component(FILE_NAME ${CAPNP} NAME)
    set(CAPNPH "${fpga-interchange-schema_SOURCE_DIR}/interchange/${FILE_NAME}.h")
    set(CAPNPCPP "${fpga-interchange-schema_SOURCE_DIR}/interchange/${FILE_NAME}.c++")
    add_custom_command(
      OUTPUT
        ${CAPNPH}
        ${CAPNPCPP}
      DEPENDS
        ${CAPNP}
        $<TARGET_FILE:capnp_tool>
        $<TARGET_FILE:capnpc_cpp>
      WORKING_DIRECTORY
        "${PROJECT_BINARY_DIR}"
      COMMAND
        $<TARGET_FILE:capnp_tool> compile
        --import-path="${capnproto-java_SOURCE_DIR}/compiler/src/main/schema"
        --import-path="$<TARGET_PROPERTY:capnp,SOURCE_DIR>/.."
        --import-path="${fpga-interchange-schema_SOURCE_DIR}/interchange"
        --output="$<TARGET_FILE:capnpc_cpp>"
        --src-prefix=${PROJECT_BINARY_DIR}
        ${CAPNP}
    )
    target_sources(interchange-files PRIVATE ${CAPNPCPP})
    target_sources(interchange-files PUBLIC ${CAPNPH})
    target_sources(interchange-files PRIVATE ${CAPNP})
  endforeach(CAPNP)

endblock()
