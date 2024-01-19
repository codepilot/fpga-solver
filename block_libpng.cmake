include_guard(GLOBAL)
block()
  set(ZLIB_INCLUDE_DIRS $<TARGET_PROPERTY:zlib,SOURCE_DIR>;$<TARGET_PROPERTY:zlib,BINARY_DIR>)
  set(ZLIB_LIBRARIES ZLIB::zlibstatic)
  set(PNG_SHARED OFF)
  set(PNG_STATIC ON)
  set(PNG_FRAMEWORK OFF)
  set(PNG_EXECUTABLES OFF)
  set(PNG_TESTS OFF)
  set(PNG_DEBUG OFF)
  set(PNG_HARDWARE_OPTIMIZATIONS ON)
  set(SKIP_INSTALL_LIBRARIES ON)
  set(SKIP_INSTALL_ALL ON)
  set(SKIP_INSTALL_HEADERS ON)
  set(SKIP_INSTALL_EXECUTABLES ON)
  set(SKIP_INSTALL_PROGRAMS ON)
  set(SKIP_INSTALL_FILES ON)
  set(SKIP_INSTALL_EXPORT ON)

  find_package(libpng)
  add_library(libpng::png_static ALIAS png_static)
endblock()