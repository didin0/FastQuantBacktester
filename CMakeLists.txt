cmake_minimum_required(VERSION 3.15)
project(FastQuantBacktester LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL REQUIRED)

# Source targets
add_library(data_loader
    src/DataLoader/CSVDataLoader.cpp
  src/DataLoader/APIDataLoader.cpp
)
target_include_directories(data_loader PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

find_package(Threads REQUIRED)
target_link_libraries(data_loader PUBLIC Threads::Threads CURL::libcurl)

# Tests
include(FetchContent)
FetchContent_Declare(
  catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v2.13.9
)
FetchContent_MakeAvailable(catch2)

FetchContent_Declare(
  fast_cpp_csv_parser
  GIT_REPOSITORY https://github.com/ben-strasser/fast-cpp-csv-parser.git
  GIT_TAG master
)
FetchContent_MakeAvailable(fast_cpp_csv_parser)

target_include_directories(data_loader PUBLIC ${fast_cpp_csv_parser_SOURCE_DIR})

FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.11.0
)
FetchContent_MakeAvailable(spdlog)

target_link_libraries(data_loader PUBLIC spdlog::spdlog)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)
target_link_libraries(data_loader PUBLIC nlohmann_json::nlohmann_json)


# Engine library (Strategy + BacktestEngine)
add_library(engine
    src/BacktestEngine/BacktestEngine.cpp
  src/Strategy/MovingAverageStrategy.cpp
  src/Strategy/BreakoutStrategy.cpp
  src/Model/Portfolio.cpp
)
target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(engine PUBLIC data_loader)

add_library(reporter
    src/Reporter/Reporter.cpp
)
target_include_directories(reporter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(reporter PUBLIC engine nlohmann_json::nlohmann_json)

add_library(app_runner
  src/App/RunConfig.cpp
  src/App/EnvLoader.cpp
)
target_include_directories(app_runner PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(app_runner PUBLIC reporter nlohmann_json::nlohmann_json)

add_executable(fastquant_cli
    src/cli/main.cpp
)
target_include_directories(fastquant_cli PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(fastquant_cli PRIVATE app_runner)

add_executable(test_csv
  tests/test_csv_loader.cpp
  tests/test_csv_stream.cpp
  tests/test_csv_strict.cpp
  tests/test_csv_earlystop.cpp
  tests/test_csv_noheader.cpp
  tests/bench_csv_perf.cpp
  tests/test_moving_average.cpp
  tests/test_engine_integration.cpp
  tests/test_portfolio.cpp
  tests/test_epoch_ms.cpp
  tests/test_reporter.cpp
    tests/test_execution_model.cpp
  tests/test_cli_config.cpp
  tests/test_breakout_strategy.cpp
  tests/test_api_loader.cpp
)
target_link_libraries(test_csv PRIVATE data_loader engine reporter app_runner Catch2::Catch2)
target_include_directories(test_csv PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

enable_testing()
add_test(NAME csv_tests COMMAND test_csv)
