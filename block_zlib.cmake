include_guard(GLOBAL)
block()
  find_package(zlib)
  add_library(ZLIB::ZLIB ALIAS zlib)
  add_library(ZLIB::zlibstatic ALIAS zlibstatic)
endblock()
