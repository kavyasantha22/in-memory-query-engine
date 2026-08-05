#include <benchmark/benchmark.h>
#include "query_engine.hpp"

static void benchmarkProjection(benchmark::State& state){
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

BENCHMARK(benchmarkProjection) 
    -> Arg(10'000) 
    -> Arg(100'000) 
    -> Arg(1'000'000)
    -> Arg(10'000'000)
    -> Arg(100'000'000)
    -> Arg(1'000'000'000)
    -> Unit(benchmark::kMillisecond)
;

BENCHMARK_MAIN();
