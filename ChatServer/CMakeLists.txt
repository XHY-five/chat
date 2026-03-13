cmake_minimum_required(VERSION 3.21)

project(ChatServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

include(cmake/Dependencies.cmake)
include(cmake/ThirdPartyTargets.cmake)

set(CHATSERVER_PROTO "${CMAKE_CURRENT_SOURCE_DIR}/proto/message.proto")

if(UNIX AND NOT APPLE)
  set(CHATSERVER_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
  file(MAKE_DIRECTORY "${CHATSERVER_GENERATED_DIR}")

  set(CHATSERVER_PROTO_SRCS
    "${CHATSERVER_GENERATED_DIR}/message.pb.cc"
    "${CHATSERVER_GENERATED_DIR}/message.grpc.pb.cc"
  )
  set(CHATSERVER_PROTO_HDRS
    "${CHATSERVER_GENERATED_DIR}/message.pb.h"
    "${CHATSERVER_GENERATED_DIR}/message.grpc.pb.h"
  )

  add_custom_command(
    OUTPUT ${CHATSERVER_PROTO_SRCS} ${CHATSERVER_PROTO_HDRS}
    COMMAND protobuf::protoc
    ARGS
      --proto_path "${CMAKE_CURRENT_SOURCE_DIR}"
      --cpp_out "${CHATSERVER_GENERATED_DIR}"
      --grpc_out "${CHATSERVER_GENERATED_DIR}"
      --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
      "${CHATSERVER_PROTO}"
    DEPENDS "${CHATSERVER_PROTO}"
    VERBATIM
  )
else()
  set(CHATSERVER_PROTO_SRCS
    source/message.pb.cc
    source/message.grpc.pb.cc
  )
  set(CHATSERVER_PROTO_HDRS
    include/message.pb.h
    include/message.grpc.pb.h
  )
endif()

set(PROJECT_SOURCES
  source/AsioIOServicePool.cpp
  source/ChatGrpcClient.cpp
  source/ChatServer.cpp
  source/ChatServiceImpl.cpp
  source/ConfigMgr.cpp
  source/CServer.cpp
  source/CSession.cpp
  source/DistLock.cpp
  source/LogicSystem.cpp
  source/MsgNode.cpp
  source/MysqlDao.cpp
  source/MysqlMgr.cpp
  source/RedisMgr.cpp
  source/StatusGrpcClient.cpp
  source/UserMgr.cpp
  ${CHATSERVER_PROTO_SRCS}
)

set(PROJECT_HEADERS
  include/AsioIOServicePool.h
  include/ChatGrpcClient.h
  include/ChatServiceImpl.h
  include/ConfigMgr.h
  include/const.h
  include/CServer.h
  include/CSession.h
  include/data.h
  include/DistLock.h
  include/LogicSystem.h
  include/MsgNode.h
  include/MysqlDao.h
  include/MysqlMgr.h
  include/RedisMgr.h
  include/Singleton.h
  include/StatusGrpcClient.h
  include/UserMgr.h
  ${CHATSERVER_PROTO_HDRS}
)

add_executable(ChatServer
  ${PROJECT_SOURCES}
  ${PROJECT_HEADERS}
)

target_compile_definitions(ChatServer PRIVATE
  UNICODE
  _UNICODE
)

if(WIN32)
  target_compile_definitions(ChatServer PRIVATE
    _WIN32_WINNT=0x0601
    BOOST_ALL_NO_LIB
  )
endif()

target_include_directories(ChatServer PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "$<$<BOOL:${UNIX}>:${CHATSERVER_GENERATED_DIR}>"
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
  "${CMAKE_CURRENT_SOURCE_DIR}/source"
)

target_link_libraries(ChatServer PRIVATE
  ChatServer::third_party
)

set_target_properties(ChatServer PROPERTIES
  VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/$<IF:$<BOOL:${CMAKE_CONFIGURATION_TYPES}>,x64/$<CONFIG>,${CMAKE_BINARY_DIR}>"
)

foreach(config Debug Release RelWithDebInfo MinSizeRel)
  string(TOUPPER "${config}" config_upper)
  set_target_properties(ChatServer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_${config_upper} "${CMAKE_CURRENT_SOURCE_DIR}/x64/${config}"
  )
endforeach()

add_custom_command(TARGET ChatServer POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
          "${CMAKE_CURRENT_SOURCE_DIR}/config/config.ini"
          "$<TARGET_FILE_DIR:ChatServer>/config.ini"
  VERBATIM
)

if(WIN32)
  add_custom_command(TARGET ChatServer POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/bin/mysqlcppconn-9-vs14.dll"
            "$<TARGET_FILE_DIR:ChatServer>/mysqlcppconn-9-vs14.dll"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/bin/mysqlcppconn8-2-vs14.dll"
            "$<TARGET_FILE_DIR:ChatServer>/mysqlcppconn8-2-vs14.dll"
    VERBATIM
  )
endif()
