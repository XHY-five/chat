set(_chatserver_default_deps_root "")

if(DEFINED ENV{CHATSERVER_DEPS_ROOT} AND NOT "$ENV{CHATSERVER_DEPS_ROOT}" STREQUAL "")
  set(_chatserver_default_deps_root "$ENV{CHATSERVER_DEPS_ROOT}")
elseif(EXISTS "${CMAKE_SOURCE_DIR}/third_party")
  set(_chatserver_default_deps_root "${CMAKE_SOURCE_DIR}/third_party")
elseif(WIN32)
  set(_chatserver_default_deps_root "E:/cppsoft")
endif()

set(CPPSOFT_ROOT "${_chatserver_default_deps_root}" CACHE PATH "Root directory of the third-party dependencies")
set(GRPC_ROOT "${CPPSOFT_ROOT}/grpc" CACHE PATH "gRPC source/prebuilt root")
set(GRPC_VISUALPRO_ROOT "${GRPC_ROOT}/visualpro" CACHE PATH "gRPC Visual Studio build output root")
set(MYSQL_CONNECTOR_ROOT "${CPPSOFT_ROOT}/mysql-connector" CACHE PATH "MySQL Connector/C++ root")
set(HIREDIS_ROOT "${CPPSOFT_ROOT}/reids" CACHE PATH "hiredis root")
set(JSON_ROOT "${CPPSOFT_ROOT}/libjsonmd" CACHE PATH "jsoncpp root")
set(BOOST_ROOT_LOCAL "${CPPSOFT_ROOT}/boost_1_89_0" CACHE PATH "Boost root")

function(chatserver_require_path path_value hint)
  if(NOT EXISTS "${path_value}")
    message(FATAL_ERROR
      "Missing required dependency path: ${path_value}\n"
      "Hint: ${hint}\n"
      "You can set CHATSERVER_DEPS_ROOT or pass -DCPPSOFT_ROOT=<path> to CMake."
    )
  endif()
endfunction()

if(WIN32)
  chatserver_require_path("${BOOST_ROOT_LOCAL}" "Place boost_1_89_0 under the dependency root.")
  chatserver_require_path("${GRPC_ROOT}/include" "Place grpc under the dependency root.")
  chatserver_require_path("${GRPC_VISUALPRO_ROOT}" "Build or copy the prebuilt grpc Visual Studio outputs under grpc/visualpro.")
  chatserver_require_path("${MYSQL_CONNECTOR_ROOT}/include" "Place mysql-connector under the dependency root.")
  chatserver_require_path("${HIREDIS_ROOT}/deps/hiredis" "Place reids under the dependency root.")
  chatserver_require_path("${JSON_ROOT}/include" "Place libjsonmd under the dependency root.")
endif()

if(CPPSOFT_ROOT)
  message(STATUS "Using CPPSOFT_ROOT=${CPPSOFT_ROOT}")
endif()
