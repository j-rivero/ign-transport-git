include (${project_cmake_dir}/Utils.cmake)

set (headers
  Discovery.hh
  HandlerStorage.hh
  Helpers.hh
  ign.hh
  NetUtils.hh
  Node.hh
  NodePrivate.hh
  NodeShared.hh
  Packet.hh
  RepHandler.hh
  ReqHandler.hh
  SubscriptionHandler.hh
  TopicStorage.hh
  TopicUtils.hh
  TransportTypes.hh
  Uuid.hh
)

#################################################
### Do not change the following

set (ign_headers "" CACHE INTERNAL "Ignition headers" FORCE)
foreach (hdr ${headers})
  APPEND_TO_CACHED_STRING(ign_headers
    "Ignition headers" "#include <ignition/${IGN_PROJECT_NAME}/${hdr}>\n")
endforeach()

configure_file (${CMAKE_CURRENT_SOURCE_DIR}/ign_auto_headers.hh.in
  ${CMAKE_CURRENT_BINARY_DIR}/${IGN_PROJECT_NAME}.hh)

ign_install_includes("" ${CMAKE_CURRENT_BINARY_DIR}/${IGN_PROJECT_NAME}.hh)
ign_install_includes(${IGN_PROJECT_NAME} ${headers})
