cmake_minimum_required(VERSION 3.25)
set(CMAKE_CXX_STANDARD 20)

set(EXECUTABLE_NAME "imapcl")
set(SOURCES
    "main.cpp"
    "connection.h"
    "connection.cpp"
    "tcp_connection.h"
    "tcp_connection.cpp"
    "ssl_connection.h"
    "ssl_connection.cpp"
    "imap_client.h"
    "imap_client.cpp"
)

find_package(OpenSSL REQUIRED)

add_executable(${EXECUTABLE_NAME})
target_sources(${EXECUTABLE_NAME} PRIVATE ${SOURCES})
target_link_libraries(${EXECUTABLE_NAME} OpenSSL::SSL OpenSSL::Crypto)