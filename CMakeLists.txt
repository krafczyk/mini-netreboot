cmake_minimum_required(VERSION 2.8)
project(mini-netreboot)

find_package(PkgConfig)

pkg_search_module(ArgParse REQUIRED ArgParse)

include_directories(${ArgParse_INCLUDEDIR})

add_executable(mini-netreboot-serve src/mini-netreboot-serve.c)
