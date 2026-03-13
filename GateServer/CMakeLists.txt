cmake_minimum_required(VERSION 3.21)

project(GateServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

include(cmake/Dependencies.cmake)
include(cmake/ThirdPartyTargets.cmake)

set(GATESERVER_PROTO "${CMAKE_CURRENT_SOURCE_DIR}/proto/message.proto")

if(UNIX AND NOT APPLE)
  set(GATESERVER_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
  file(MAKE_DIRECTORY "${GATESERVER_GENERATED_DIR}")

  set(GATESERVER_PROTO_SRCS
    "${GATESERVER_GENERATED_DIR}/message.pb.cc"
    "${GATESERVER_GENERATED_DIR}/message.grpc.pb.cc"
  )
  set(GATESERVER_PROTO_HDRS
    "${GATESERVER_GENERATED_DIR}/message.pb.h"
    "${GATESERVER_GENERATED_DIR}/message.grpc.pb.h"
  )

  add_custom_command(
    OUTPUT ${GATESERVER_PROTO_SRCS} ${GATESERVER_PROTO_HDRS}
    COMMAND protobuf::protoc
    ARGS
      --proto_path "${CMAKE_CURRENT_SOURCE_DIR}/proto"
      --cpp_out "${GATESERVER_GENERATED_DIR}"
      --grpc_out "${GATESERVER_GENERATED_DIR}"
      --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
      "${GATESERVER_PROTO}"
    DEPENDS "${GATESERVER_PROTO}"
    VERBATIM
  )
else()
  set(GATESERVER_PROTO_SRCS
    source/message.pb.cc
    source/message.grpc.pb.cc
  )
  set(GATESERVER_PROTO_HDRS
    include/message.pb.h
    include/message.grpc.pb.h
  )
endif()

set(PROJECT_SOURCES
  source/AsioIOServicePool.cpp
  source/ConfigMgr.cpp
  source/CServer.cpp
  source/GateServer.cpp
  source/HttpConnection.cpp
  source/LogicSystem.cpp
  source/MysqlDao.cpp
  source/MysqlMgr.cpp
  source/RedisMgr.cpp
  source/StatusGrpcClient.cpp
  source/VerifyGrpcClient.cpp
  ${GATESERVER_PROTO_SRCS}
)

set(PROJECT_HEADERS
  include/AsioIOServicePool.h
  include/ConfigMgr.h
  include/const.h
  include/CServer.h
  include/HttpConnection.h
  include/LogicSystem.h
  include/MysqlDao.h
  include/MysqlMgr.h
  include/RedisMgr.h
  include/Singleton.h
  include/StatusGrpcClient.h
  include/VerifyGrpcClient.h
  ${GATESERVER_PROTO_HDRS}
)

add_executable(GateServer
  ${PROJECT_SOURCES}
  ${PROJECT_HEADERS}
)

target_compile_definitions(GateServer PRIVATE
  UNICODE
  _UNICODE
)

if(WIN32)
  target_compile_definitions(GateServer PRIVATE
    _WIN32_WINNT=0x0601
    BOOST_ALL_NO_LIB
  )
endif()

target_include_directories(GateServer PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "$<$<BOOL:${UNIX}>:${GATESERVER_GENERATED_DIR}>"
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/source"
)

target_link_libraries(GateServer PRIVATE
  GateServer::third_party
)

set_target_properties(GateServer PROPERTIES
  VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/$<IF:$<BOOL:${CMAKE_CONFIGURATION_TYPES}>,x64/$<CONFIG>,${CMAKE_BINARY_DIR}>"
)

foreach(config Debug Release RelWithDebInfo MinSizeRel)
  string(TOUPPER "${config}" config_upper)
  set_target_properties(GateServer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_${config_upper} "${CMAKE_CURRENT_SOURCE_DIR}/x64/${config}"
  )
endforeach()

add_custom_command(TARGET GateServer POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
          "${CMAKE_CURRENT_SOURCE_DIR}/config/config.ini"
          "$<TARGET_FILE_DIR:GateServer>/config.ini"
  VERBATIM
)

if(WIN32)
  add_custom_command(TARGET GateServer POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/bin/mysqlcppconn-9-vs14.dll"
            "$<TARGET_FILE_DIR:GateServer>/mysqlcppconn-9-vs14.dll"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/bin/mysqlcppconn8-2-vs14.dll"
            "$<TARGET_FILE_DIR:GateServer>/mysqlcppconn8-2-vs14.dll"
    VERBATIM
  )
endif()
