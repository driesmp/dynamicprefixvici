cmake_minimum_required(VERSION 3.0.0)
project(dynamicprefixvici VERSION 0.1.0)

include(CTest)
enable_testing()

add_compile_options(-Wall -Wextra -pedantic -march=native -std=gnu2x) #-Weverything -fanalyzer

add_executable(dynamicprefixvici dynamicprefixvici.c)

target_link_libraries(dynamicprefixvici /usr/local/lib/ipsec/libvici.so)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
