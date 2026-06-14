cmake_minimum_required(VERSION 3.10)
project(webserver VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ===================== 可配置开关 =====================
option(USE_ASAN "Enable AddressSanitizer for memory check" OFF)

# ===================== 通用编译配置 =====================
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type")
endif()

# ===================== 分支配置：ASAN / 性能版 =====================
if(USE_ASAN)
    message(STATUS "===== Build with ASAN (Memory Check Mode) =====")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -ggdb3")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -ggdb3")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")

else()
    message(STATUS "===== Build for Performance & Perf Sampling =====")
    # RelWithDebInfo 已自带 -O2，不再重复添加
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -ggdb3")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-omit-frame-pointer -ggdb3")
endif()

# ===================== 源码 & 目标 =====================
set(WEBSERVER_SRCS
    acceptor.cpp
    channel.cpp
    epoller.cpp
    eventloop.cpp
    eventloop_thread.cpp
    eventloop_thread_pool.cpp
    tcp_connection.cpp
    tcp_server.cpp
    buffer.cpp
    timestamp.cpp
)

add_library(webserver_lib STATIC ${WEBSERVER_SRCS})

add_executable(echo_server example/echo_server.cpp)
add_executable(echo_client example/echo_client.cpp)

target_include_directories(echo_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(echo_server webserver_lib pthread)