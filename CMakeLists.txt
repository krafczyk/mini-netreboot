cmake_minimum_required(VERSION 2.8)
project(mini-netreboot)

find_package(PkgConfig)

add_definitions("-std=c++11")

include_directories(inc)

add_executable(mini-netreboot-serve src/mini-netreboot-serve.cpp)
add_executable(mini-netreboot-read src/mini-netreboot-read.cpp)

target_link_libraries(mini-netreboot-serve ${ArgParse_LDFLAGS})
target_link_libraries(mini-netreboot-read ${ArgParse_LDFLAGS})

install(TARGETS mini-netreboot-serve mini-netreboot-read DESTINATION bin)
install(FILES conf/mini-netreboot.conf DESTINATION etc/mini-netreboot)
install(FILES service/mini-netreboot-serve.service service/mini-netreboot-read.service DESTINATION lib/systemd/system)
