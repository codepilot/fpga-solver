include_guard(GLOBAL)
block()
  FetchContent_Declare(zlib GIT_REPOSITORY https://github.com/madler/zlib.git GIT_TAG v1.2.11 OVERRIDE_FIND_PACKAGE)
  find_package(zlib)
  add_library(ZLIB::ZLIB ALIAS zlib)
  add_library(ZLIB::zlibstatic ALIAS zlibstatic)

  if(TARGET example64)
    set_target_properties(example64 PROPERTIES EXCLUDE_FROM_ALL TRUE)
  endif()

  if(TARGET minigzip64)
    set_target_properties(minigzip64 PROPERTIES EXCLUDE_FROM_ALL TRUE)
  endif()

  set_target_properties(
    example
    minigzip
    zlib
  PROPERTIES
    EXCLUDE_FROM_ALL TRUE
  )

endblock()
