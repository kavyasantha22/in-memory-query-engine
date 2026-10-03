#include "benchmark_common.hpp"

#include <benchmark/benchmark.h>

namespace {

void bmAggregateCount(benchmark::State& state){
    benchmarkSupport::runAggregation(
        state,
        query_engine::Aggregation{.type = query_engine::AggregationType::COUNT, .column = query_engine::ColumnName::PRICE}
    );
}

void bmAggregateSum(benchmark::State& state){
    benchmarkSupport::runAggregation(
        state,
        query_engine::Aggregation{.type = query_engine::AggregationType::SUM, .column = query_engine::ColumnName::PRICE}
    );
}

void bmAggregateAverage(benchmark::State& state){
    benchmarkSupport::runAggregation(
        state,
        query_engine::Aggregation{.type = query_engine::AggregationType::AVG, .column = query_engine::ColumnName::PRICE}
    );
}

void bmAggregateMinimum(benchmark::State& state){
    benchmarkSupport::runAggregation(
        state,
        query_engine::Aggregation{.type = query_engine::AggregationType::MIN, .column = query_engine::ColumnName::PRICE}
    );
}

void bmAggregateMaximum(benchmark::State& state){
    benchmarkSupport::runAggregation(
        state,
        query_engine::Aggregation{.type = query_engine::AggregationType::MAX, .column = query_engine::ColumnName::PRICE}
    );
}

#define REGISTER_AGGREGATION(functionName) \
    BENCHMARK(functionName) \
        ->Apply(benchmarkSupport::addStandardRowCounts) \
        ->Unit(benchmark::kMillisecond) \
        ->Complexity(benchmark::oN)

REGISTER_AGGREGATION(bmAggregateCount);
REGISTER_AGGREGATION(bmAggregateSum);
REGISTER_AGGREGATION(bmAggregateAverage);
REGISTER_AGGREGATION(bmAggregateMinimum);
REGISTER_AGGREGATION(bmAggregateMaximum);

#undef REGISTER_AGGREGATION

} // namespace
