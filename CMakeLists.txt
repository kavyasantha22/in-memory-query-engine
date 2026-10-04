cmake_minimum_required(VERSION 3.20)

project(QueryEngine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(query_engine
    src/aggregation.cpp
    src/formatter.cpp
    src/query.cpp
    src/table.cpp
    src/util.cpp
)

target_include_directories(query_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(MSVC)
    target_compile_options(query_engine PRIVATE /W4)
else()
    target_compile_options(query_engine PRIVATE -Wall -Wextra)
endif()

add_executable(random_query examples/random.cpp)
target_link_libraries(random_query PRIVATE query_engine)

include(CTest)

if(BUILD_TESTING)
    function(add_query_engine_test target source)
        add_executable(${target} ${source})
        target_link_libraries(${target} PRIVATE query_engine)
        add_test(NAME ${target} COMMAND ${target})
    endfunction()

    add_query_engine_test(query_test tests/query_test.cpp)
    add_query_engine_test(query_aggregation_test tests/query_aggregation_test.cpp)
    add_query_engine_test(aggregation_test tests/aggregation_test.cpp)
    add_query_engine_test(group_by_test tests/group_by_test.cpp)
    add_query_engine_test(order_by_test tests/order_by_test.cpp)
    add_query_engine_test(hidden_order_by_test tests/hidden_order_by_test.cpp)
    add_query_engine_test(limit_test tests/limit_test.cpp)
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

    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
        GIT_SHALLOW TRUE
    )
    set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
    set(JSON_Install OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(nlohmann_json)

    function(configure_query_engine_benchmark target)
        target_link_libraries(
            ${target}
            PRIVATE
                query_engine
                benchmark::benchmark_main
        )
    endfunction()

    add_executable(
        query_engine_benchmark
        benchmarks/aggregation_benchmark.cpp
        benchmarks/insert_benchmark.cpp
        benchmarks/query_benchmark.cpp
    )
    configure_query_engine_benchmark(query_engine_benchmark)

    add_executable(
        query_engine_general_benchmark
        benchmarks/aggregation_benchmark.cpp
        benchmarks/general_benchmark_main.cpp
        benchmarks/insert_benchmark.cpp
        benchmarks/query_benchmark.cpp
    )
    target_link_libraries(
        query_engine_general_benchmark
        PRIVATE
            query_engine
            benchmark::benchmark
            nlohmann_json::nlohmann_json
    )
    if(MSVC)
        target_compile_options(query_engine_general_benchmark PRIVATE /W4)
    else()
        target_compile_options(query_engine_general_benchmark PRIVATE -Wall -Wextra)
    endif()

    add_executable(
        query_engine_stress_benchmark
        benchmarks/stress_benchmark.cpp
    )
    configure_query_engine_benchmark(query_engine_stress_benchmark)

    set(BENCHMARK_RESULTS_DIR ${CMAKE_CURRENT_BINARY_DIR}/benchmark-results)

    add_custom_target(
        run_general_benchmarks
        COMMAND ${CMAKE_COMMAND} -E make_directory ${BENCHMARK_RESULTS_DIR}
        COMMAND
            $<TARGET_FILE:query_engine_general_benchmark>
            --general_summary=${BENCHMARK_RESULTS_DIR}/general-latest.json
            --general_baseline=${BENCHMARK_RESULTS_DIR}/general-best.json
            --benchmark_min_time=0.1s
            --benchmark_repetitions=5
            --benchmark_enable_random_interleaving
            --benchmark_report_aggregates_only=true
        DEPENDS query_engine_general_benchmark
        USES_TERMINAL
        VERBATIM
    )

    add_custom_target(
        save_general_benchmark_baseline
        COMMAND ${CMAKE_COMMAND} -E make_directory ${BENCHMARK_RESULTS_DIR}
        COMMAND
            $<TARGET_FILE:query_engine_general_benchmark>
            --general_summary=${BENCHMARK_RESULTS_DIR}/general-baseline.json
            --benchmark_min_time=0.1s
            --benchmark_repetitions=5
            --benchmark_enable_random_interleaving
            --benchmark_report_aggregates_only=true
        COMMAND
            ${CMAKE_COMMAND} -E echo
            "Baseline: ${BENCHMARK_RESULTS_DIR}/general-baseline.json"
        DEPENDS query_engine_general_benchmark
        USES_TERMINAL
        VERBATIM
    )

    add_custom_target(
        run_benchmarks
        COMMAND ${CMAKE_COMMAND} -E make_directory ${BENCHMARK_RESULTS_DIR}
        COMMAND
            $<TARGET_FILE:query_engine_benchmark>
            --benchmark_min_time=0.1s
            --benchmark_repetitions=5
            --benchmark_enable_random_interleaving
            --benchmark_display_aggregates_only=true
            --benchmark_out=${BENCHMARK_RESULTS_DIR}/latest.json
            --benchmark_out_format=json
        DEPENDS query_engine_benchmark
        USES_TERMINAL
    )

    add_custom_target(
        run_stress_benchmarks
        COMMAND ${CMAKE_COMMAND} -E make_directory ${BENCHMARK_RESULTS_DIR}
        COMMAND
            $<TARGET_FILE:query_engine_stress_benchmark>
            --benchmark_min_time=0.1s
            --benchmark_repetitions=3
            --benchmark_enable_random_interleaving
            --benchmark_display_aggregates_only=true
            --benchmark_out=${BENCHMARK_RESULTS_DIR}/stress-latest.json
            --benchmark_out_format=json
        DEPENDS query_engine_stress_benchmark
        USES_TERMINAL
    )
endif()
