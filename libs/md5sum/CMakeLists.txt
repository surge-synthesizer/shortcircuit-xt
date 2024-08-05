cmake_minimum_required(VERSION 3.10)
project(md5sum LANGUAGES CXX)

if ("${CMAKE_VERSION}" VERSION_GREATER_EQUAL "3.13")
  cmake_policy(SET CMP0077 NEW)
endif()

file(GLOB MD5_SRC src/impl/*.cc)
add_library(md5sum STATIC ${MD5_SRC})

if (MSVC)
  set(MD5_COMPILE_OPTIONS /GR-)
else()
  set(MD5_COMPILE_OPTIONS
          -fno-rtti -fno-exceptions
          -fno-unwind-tables -fno-asynchronous-unwind-tables)
endif()

target_compile_options(md5sum PRIVATE ${MD5_COMPILE_OPTIONS})
target_include_directories(md5sum PUBLIC src/)

add_library(md5sum::md5 ALIAS md5sum)
