#include <benchmark/benchmark.h>
#include "query_engine.hpp"

static void bmProjection(benchmark::State& state){
    const uint64_t row_count = state.range(0);

    Table table = generateTable(row_count);

    Query query {
        .projection = {ColumnName::PRICE}
    };

    for (auto _: state){
        ResultTable result = queryTable(table, query);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(
        state.iterations() * (row_count)
    );
}

static void bmInsert(benchmark::State& state){
    const uint64_t row_count = state.range(0);

    Table table = generateTable(row_count);

    Row row {
        .transaction_id = 23123123,
        .product_id = 19,
        .category_id = 1,
        .price = 100,
        .quantity = 5,
        .timestamp = 100000
    };
    for (auto _: state){
        insertRow(table, row);
        benchmark::DoNotOptimize(table.rows.data());
    }

    state.SetItemsProcessed(
        state.iterations() * 1
    );
}

BENCHMARK(bmInsert)
      ->Arg(10'000)
      ->Arg(100'000)
      ->Arg(1'000'000)
      ->Unit(benchmark::kNanosecond);

BENCHMARK(bmProjection) 
    -> Arg(10'000) 
    -> Arg(100'000) 
    -> Arg(1'000'000)
    -> Arg(10'000'000)
    -> Unit(benchmark::kMillisecond);

BENCHMARK_MAIN();
