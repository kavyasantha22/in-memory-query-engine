#pragma once

#include "query_engine/query_engine.hpp"

#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <functional>
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

inline query_engine::Row makeRow(){
    return query_engine::Row{
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
    const query_engine::Query& query,
    std::size_t expectedRows
){
    const std::int64_t rowCount = state.range(0);
    const query_engine::Table table = query_engine::generateTable(static_cast<std::uint64_t>(rowCount));
    std::size_t resultRows = 0;

    for (auto _ : state){
        query_engine::ResultTable result = query_engine::queryTable(table, query);
        resultRows = result.rows.size();
        benchmark::DoNotOptimize(result);
    }

    if (resultRows != expectedRows){
        state.SkipWithError("query returned an unexpected row count");
    }
    recordRows(state, rowCount);
}

inline void runAggregation(benchmark::State& state, query_engine::Aggregation aggregation){
    const std::int64_t rowCount = state.range(0);
    const query_engine::Table table = query_engine::generateTable(static_cast<std::uint64_t>(rowCount));
    std::vector<std::reference_wrapper<const query_engine::Row>> rows;
    rows.reserve(table.rows.size());
    for (const query_engine::Row& row: table.rows){
        rows.push_back(std::cref(row));
    }

    for (auto _ : state){
        query_engine::ResultValue result = query_engine::aggregate(rows, aggregation);
        benchmark::DoNotOptimize(result);
    }

    recordRows(state, rowCount);
}

inline void runBatchInsert(benchmark::State& state, bool reserveCapacity){
    const std::int64_t rowCount = state.range(0);
    const query_engine::Row row = makeRow();

    for (auto _ : state){
        state.PauseTiming();
        bool sizeIsValid = false;
        {
            query_engine::Table table = query_engine::generateTable(0);
            if (reserveCapacity){
                table.rows.reserve(static_cast<std::size_t>(rowCount));
            }

            state.ResumeTiming();
            for (std::int64_t i = 0; i < rowCount; ++i){
                query_engine::insertRow(table, row);
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
