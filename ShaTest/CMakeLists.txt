# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.8)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(hello_world)

# Select C++20.
# use c++20 standard for building. c++17 isn't enough because of:
# https://github.com/espressif/esp-idf/pull/6692
# https://github.com/espressif/esp-idf/issues/5897
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)