cmake_minimum_required(VERSION 3.20)

project(QueryEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(query_engine
    aggregation.cpp
    formatter.cpp
    query.cpp
    table.cpp
)

target_include_directories(query_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(MSVC)
    target_compile_options(query_engine PRIVATE /W4)
else()
    target_compile_options(query_engine PRIVATE -Wall -Wextra)
endif()

add_executable(random_query random.cpp)
target_link_libraries(random_query PRIVATE query_engine)

include(CTest)

if(BUILD_TESTING)
    function(add_query_engine_test target source)
        add_executable(${target} ${source})
        target_link_libraries(${target} PRIVATE query_engine)
        add_test(NAME ${target} COMMAND ${target})
    endfunction()

    add_query_engine_test(query_test correctness_test/query_test.cpp)
    add_query_engine_test(query_aggregation_test correctness_test/query_aggregation_test.cpp)
    add_query_engine_test(aggregation_test correctness_test/aggregation_test.cpp)
    add_query_engine_test(group_by_test correctness_test/group_by_test.cpp)
    add_query_engine_test(order_by_test correctness_test/order_by_test.cpp)
    add_query_engine_test(hidden_order_by_test correctness_test/hidden_order_by_test.cpp)
    add_query_engine_test(limit_test correctness_test/limit_test.cpp)
endif()

option(QUERY_ENGINE_BUILD_BENCHMARKS "Build Google Benchmark targets" ON)

if(QUERY_ENGINE_BUILD_BENCHMARKS)
    include(FetchContent)

    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_INSTALL_DOCS OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        google_benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.9.5
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(google_benchmark)

    add_executable(query_engine_benchmark benchmark.cpp)
    target_link_libraries(
        query_engine_benchmark
        PRIVATE
            query_engine
            benchmark::benchmark_main
    )
endif()
