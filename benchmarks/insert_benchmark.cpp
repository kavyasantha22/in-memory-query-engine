#include "benchmark_common.hpp"

#include <benchmark/benchmark.h>

namespace {

void bmInsertReserved(benchmark::State& state){
    benchmarkSupport::runBatchInsert(state, true);
}

void bmInsertUnreserved(benchmark::State& state){
    benchmarkSupport::runBatchInsert(state, false);
}

#define REGISTER_INSERT(functionName) \
    BENCHMARK(functionName) \
        ->Apply(benchmarkSupport::addInsertRowCounts) \
        ->Unit(benchmark::kMillisecond) \
        ->Complexity(benchmark::oN)

REGISTER_INSERT(bmInsertReserved);
REGISTER_INSERT(bmInsertUnreserved);

#undef REGISTER_INSERT

} // namespace
