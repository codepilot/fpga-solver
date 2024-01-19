include_guard(GLOBAL)
block()
  set(BUILD_DEV_ONLY ON)
  set(BUILD_DISPATCHER_ONLY                OFF)
  set(BUILD_DEV                            ON)
  set(BUILD_DISPATCHER                     OFF)
  set(BUILD_TOOLS                          OFF)
  set(BUILD_SHARED_LIBS                    ON)
  set(BUILD_TESTS                          OFF)
  set(BUILD_EXAMPLES                       OFF)
  set(BUILD_PREVIEW                        OFF)
  set(BUILD_DISPATCHER_ONEVPL_EXPERIMENTAL OFF)
  set(BUILD_TOOLS_ONEVPL_EXPERIMENTAL      OFF)
  set(INSTALL_EXAMPLE_CODE                 OFF)
  set(USE_MSVC_STATIC_RUNTIME              OFF)
  set(TOOLS_ENABLE_SCREEN_CAPTURE          OFF)
  set(TOOLS_ENABLE_RENDER                  OFF)
  set(TOOLS_ENABLE_OPENCL                  OFF)
  set(TOOLS_ENABLE_X11                     OFF)

  # set(PNG_BUILD_ZLIB ON)


  # # add_dependencies(kj-gzip zlibstatic)
  # # target_link_libraries(kj-heavy-tests zlibstatic)
  # # # message(${ZLIB_LIBRARY})
  # # message(${ZLIB_INCLUDE_DIR})
  # # set(TCLAP_INCLUDE_PATH "tclap")
  # # set(Stb_INCLUDE_PATH "stb")
  # # set(OpenCL_DIR OpenCL-Headers)
  # # add_subdirectory(libvpl)
endblock()