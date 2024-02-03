include_guard(GLOBAL)
block()
  set(WITH_ZLIB off)
  set(CAPNP_LITE off)
  set(WITH_OPENSSL off)
  set(BUILD_TESTING off)
  find_package(capnproto)
  if(MSVC)
    target_link_libraries(kj Ws2_32)
  endif()

  set_target_properties(
    capnp-websocket
    kj-http
    capnp-rpc
    kj-test
  PROPERTIES
    EXCLUDE_FROM_ALL TRUE
  )

endblock()
