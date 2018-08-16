cmake_minimum_required(VERSION 2.8)
project(mini-netreboot)

find_package(PkgConfig)

if(NOT FINAL_INSTALL_PREFIX)
set(FINAL_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
endif()

add_definitions("-std=c++11")

include_directories(${CMAKE_SOURCE_DIR}/inc ${CMAKE_BINARY_DIR}/inc)

configure_file(${CMAKE_SOURCE_DIR}/inc/config.h.in ${CMAKE_BINARY_DIR}/inc/config.h)

add_executable(mini-netreboot-serve src/mini-netreboot-serve.cpp)
add_executable(mini-netreboot-read src/mini-netreboot-read.cpp)

target_link_libraries(mini-netreboot-serve ${ArgParse_LDFLAGS})
target_link_libraries(mini-netreboot-read ${ArgParse_LDFLAGS})

install(TARGETS mini-netreboot-serve mini-netreboot-read DESTINATION bin)
install(FILES conf/mini-netreboot.conf DESTINATION etc/mini-netreboot)
install(FILES service/mini-netreboot-serve.service service/mini-netreboot-read.service DESTINATION lib/systemd/system)
