cmake_minimum_required(VERSION 3.14)
project(warden LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Curses REQUIRED)

add_library(
  warden_core
  src/config.cpp
  src/process.cpp
  src/kill_utils.cpp
)
target_include_directories(warden_core PUBLIC src)

add_executable(warden src/display.cpp)
target_link_libraries(warden PRIVATE warden_core ${CURSES_LIBRARIES})
target_include_directories(warden PRIVATE ${CURSES_INCLUDE_DIR})

include(CTest)
if(BUILD_TESTING)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  )
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)

  add_executable(
    warden_tests
    tests/config_tests.cpp
    tests/process_tests.cpp
    tests/kill_utils_tests.cpp
  )
  target_link_libraries(warden_tests PRIVATE warden_core GTest::gtest_main)

  include(GoogleTest)
  gtest_discover_tests(warden_tests)
endif()
