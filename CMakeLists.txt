cmake_minimum_required(VERSION 2.8)
project(mini-netreboot)

find_package(PkgConfig)

pkg_search_module(ArgParse REQUIRED ArgParse)

include_directories(${ArgParse_INCLUDEDIR})

add_executable(mini-netreboot-serve src/mini-netreboot-serve.cpp)
add_executable(mini-netreboot-read src/mini-netreboot-read.cpp)

target_link_libraries(mini-netreboot-serve ${ArgParse_LDFLAGS})
target_link_libraries(mini-netreboot-read ${ArgParse_LDFLAGS})

install(TARGETS mini-netreboot-serve mini-netreboot-read DESTINATION bin)
