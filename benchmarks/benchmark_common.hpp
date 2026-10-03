#pragma once

#include "query_engine/query_engine.hpp"

#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace benchmarkSupport {

inline void addStandardRowCounts(benchmark::Benchmark* benchmark){
    benchmark->Arg(10'000)->Arg(100'000)->Arg(1'000'000);
}

inline void addSortRowCounts(benchmark::Benchmark* benchmark){
    benchmark->Arg(10'000)->Arg(100'000);
}

inline void addHighCardinalityRowCounts(benchmark::Benchmark* benchmark){
    benchmark->Arg(1'000)->Arg(10'000);
}

inline void addInsertRowCounts(benchmark::Benchmark* benchmark){
    benchmark->Arg(1'000)->Arg(10'000)->Arg(100'000)->Arg(1'000'000);
}

inline Row makeRow(){
    return Row{
        .transaction_id = 23'123'123,
        .product_id = 19,
        .category_id = 1,
        .price = 100,
        .quantity = 5,
        .timestamp = 100'000
    };
}

inline void recordRows(benchmark::State& state, std::int64_t rowCount){
    state.SetItemsProcessed(state.iterations() * rowCount);
    state.SetComplexityN(rowCount);
}

inline void runQuery(
    benchmark::State& state,
    Query query,
    std::size_t expectedRows
){
    const std::int64_t rowCount = state.range(0);
    const Table table = generateTable(static_cast<std::uint64_t>(rowCount));
    std::size_t resultRows = 0;

    for (auto _ : state){
        ResultTable result = queryTable(table, query);
        resultRows = result.rows.size();
        benchmark::DoNotOptimize(result);
    }

    if (resultRows != expectedRows){
        state.SkipWithError("query returned an unexpected row count");
    }
    recordRows(state, rowCount);
}

inline void runAggregation(benchmark::State& state, Aggregation aggregation){
    const std::int64_t rowCount = state.range(0);
    const Table table = generateTable(static_cast<std::uint64_t>(rowCount));

    for (auto _ : state){
        ResultValue result = aggregate(table.rows, aggregation);
        benchmark::DoNotOptimize(result);
    }

    recordRows(state, rowCount);
}

inline void runBatchInsert(benchmark::State& state, bool reserveCapacity){
    const std::int64_t rowCount = state.range(0);
    const Row row = makeRow();

    for (auto _ : state){
        state.PauseTiming();
        bool sizeIsValid = false;
        {
            Table table = generateTable(0);
            if (reserveCapacity){
                table.rows.reserve(static_cast<std::size_t>(rowCount));
            }

            state.ResumeTiming();
            for (std::int64_t i = 0; i < rowCount; ++i){
                insertRow(table, row);
            }
            benchmark::DoNotOptimize(table.rows.data());
            benchmark::ClobberMemory();
            state.PauseTiming();

            sizeIsValid = table.rows.size() == static_cast<std::size_t>(rowCount);
        }
        state.ResumeTiming();

        if (!sizeIsValid){
            state.SkipWithError("insert benchmark produced an unexpected row count");
            break;
        }
    }

    recordRows(state, rowCount);
}

} // namespace benchmarkSupport
