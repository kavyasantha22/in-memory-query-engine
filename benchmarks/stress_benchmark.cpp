#include "benchmark_common.hpp"

#include <benchmark/benchmark.h>

#include <cstddef>
#include <optional>

namespace {

constexpr std::int64_t stressRowCount = 10'000'000;

void bmStressProjection(benchmark::State& state){
    Query query{
        .projection = {ColumnName::PRICE},
        .filter = std::nullopt,
        .aggregation = std::nullopt,
        .group_by = std::nullopt,
        .order_by = std::nullopt,
        .limit = std::nullopt
    };
    benchmarkSupport::runQuery(
        state,
        query,
        static_cast<std::size_t>(state.range(0))
    );
}

void bmStressFilterTenPercent(benchmark::State& state){
    Query query{
        .projection = {ColumnName::TRANSACTION_ID},
        .filter = [](Row row){ return row.category_id == 0; },
        .aggregation = std::nullopt,
        .group_by = std::nullopt,
        .order_by = std::nullopt,
        .limit = std::nullopt
    };
    benchmarkSupport::runQuery(
        state,
        query,
        static_cast<std::size_t>(state.range(0) / 10)
    );
}

void bmStressAggregateSum(benchmark::State& state){
    benchmarkSupport::runAggregation(
        state,
        Aggregation{.type = AggregationType::SUM, .column = ColumnName::PRICE}
    );
}

void bmStressInsertReserved(benchmark::State& state){
    benchmarkSupport::runBatchInsert(state, true);
}

void bmStressInsertUnreserved(benchmark::State& state){
    benchmarkSupport::runBatchInsert(state, false);
}

#define REGISTER_STRESS(functionName) \
    BENCHMARK(functionName) \
        ->Arg(stressRowCount) \
        ->Unit(benchmark::kMillisecond)

REGISTER_STRESS(bmStressProjection);
REGISTER_STRESS(bmStressFilterTenPercent);
REGISTER_STRESS(bmStressAggregateSum);
REGISTER_STRESS(bmStressInsertReserved);
REGISTER_STRESS(bmStressInsertUnreserved);

#undef REGISTER_STRESS

} // namespace
