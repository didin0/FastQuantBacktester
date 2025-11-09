cmake_minimum_required(VERSION 3.15)
project(FastQuantBacktester LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Source targets
add_library(data_loader
    src/DataLoader/CSVDataLoader.cpp
)
target_include_directories(data_loader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

find_package(Threads REQUIRED)
target_link_libraries(data_loader PUBLIC Threads::Threads)

# Tests
include(FetchContent)
FetchContent_Declare(
  catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v2.13.9
)
FetchContent_MakeAvailable(catch2)

add_executable(test_csv tests/test_csv_loader.cpp)
target_link_libraries(test_csv PRIVATE data_loader Catch2::Catch2)
target_include_directories(test_csv PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

enable_testing()
add_test(NAME csv_tests COMMAND test_csv)
