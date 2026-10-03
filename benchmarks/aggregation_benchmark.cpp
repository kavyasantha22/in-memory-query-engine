#include "benchmark_common.hpp"

#include <benchmark/benchmark.h>

namespace {

void bmAggregateCount(benchmark::State& state){
    benchmarkSupport::runAggregation(
        state,
        Aggregation{.type = AggregationType::COUNT, .column = ColumnName::PRICE}
    );
}

void bmAggregateSum(benchmark::State& state){
    benchmarkSupport::runAggregation(
        state,
        Aggregation{.type = AggregationType::SUM, .column = ColumnName::PRICE}
    );
}

void bmAggregateAverage(benchmark::State& state){
    benchmarkSupport::runAggregation(
        state,
        Aggregation{.type = AggregationType::AVG, .column = ColumnName::PRICE}
    );
}

void bmAggregateMinimum(benchmark::State& state){
    benchmarkSupport::runAggregation(
        state,
        Aggregation{.type = AggregationType::MIN, .column = ColumnName::PRICE}
    );
}

void bmAggregateMaximum(benchmark::State& state){
    benchmarkSupport::runAggregation(
        state,
        Aggregation{.type = AggregationType::MAX, .column = ColumnName::PRICE}
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
