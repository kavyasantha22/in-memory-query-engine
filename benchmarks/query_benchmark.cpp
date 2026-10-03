#include "benchmark_common.hpp"

#include <benchmark/benchmark.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace {

using benchmarkSupport::runQuery;

query_engine::Query makeDetailQuery(std::vector<query_engine::ColumnName> projection){
    return query_engine::Query{
        .projection = projection,
        .filter = std::nullopt,
        .aggregation = std::nullopt,
        .group_by = std::nullopt,
        .order_by = std::nullopt,
        .limit = std::nullopt
    };
}

void bmProjectionOneColumn(benchmark::State& state){
    runQuery(
        state,
        makeDetailQuery({query_engine::ColumnName::PRICE}),
        static_cast<std::size_t>(state.range(0))
    );
}

void bmProjectionThreeColumns(benchmark::State& state){
    runQuery(
        state,
        makeDetailQuery({
            query_engine::ColumnName::TRANSACTION_ID,
            query_engine::ColumnName::PRODUCT_ID,
            query_engine::ColumnName::PRICE
        }),
        static_cast<std::size_t>(state.range(0))
    );
}

void bmProjectionAllColumns(benchmark::State& state){
    runQuery(
        state,
        makeDetailQuery({
            query_engine::ColumnName::TRANSACTION_ID,
            query_engine::ColumnName::PRODUCT_ID,
            query_engine::ColumnName::CATEGORY_ID,
            query_engine::ColumnName::PRICE,
            query_engine::ColumnName::QUANTITY,
            query_engine::ColumnName::TIMESTAMP
        }),
        static_cast<std::size_t>(state.range(0))
    );
}

void runFilter(
    benchmark::State& state,
    std::function<bool(const query_engine::Row&)> filter,
    std::size_t expectedRows
){
    query_engine::Query query = makeDetailQuery({query_engine::ColumnName::TRANSACTION_ID});
    query.filter = filter;
    runQuery(state, query, expectedRows);
}

void bmFilterZeroPercent(benchmark::State& state){
    runFilter(state, [](const query_engine::Row&){ return false; }, 0);
}

void bmFilterOnePercent(benchmark::State& state){
    runFilter(
        state,
        [](const query_engine::Row& row){ return row.transaction_id % 100 == 0; },
        static_cast<std::size_t>(state.range(0) / 100)
    );
}

void bmFilterTenPercent(benchmark::State& state){
    runFilter(
        state,
        [](const query_engine::Row& row){ return row.category_id == 0; },
        static_cast<std::size_t>(state.range(0) / 10)
    );
}

void bmFilterFiftyPercent(benchmark::State& state){
    runFilter(
        state,
        [](const query_engine::Row& row){ return row.transaction_id % 2 == 0; },
        static_cast<std::size_t>(state.range(0) / 2)
    );
}

void bmFilterOneHundredPercent(benchmark::State& state){
    runFilter(
        state,
        [](const query_engine::Row&){ return true; },
        static_cast<std::size_t>(state.range(0))
    );
}

void bmLimitZero(benchmark::State& state){
    query_engine::Query query = makeDetailQuery({query_engine::ColumnName::TRANSACTION_ID});
    query.limit = 0;
    runQuery(state, query, 0);
}

void bmLimitTen(benchmark::State& state){
    query_engine::Query query = makeDetailQuery({query_engine::ColumnName::TRANSACTION_ID});
    query.limit = 10;
    runQuery(
        state,
        query,
        std::min<std::size_t>(10, static_cast<std::size_t>(state.range(0)))
    );
}

void bmOrderByVisibleColumn(benchmark::State& state){
    query_engine::Query query = makeDetailQuery({query_engine::ColumnName::TRANSACTION_ID, query_engine::ColumnName::PRICE});
    query.order_by = std::vector<query_engine::OrderByItem>{
        query_engine::OrderByItem{.expr = query_engine::ColumnName::PRICE, .ascending = true}
    };
    runQuery(state, query, static_cast<std::size_t>(state.range(0)));
}

void bmOrderByMultipleColumns(benchmark::State& state){
    query_engine::Query query = makeDetailQuery({
        query_engine::ColumnName::TRANSACTION_ID,
        query_engine::ColumnName::CATEGORY_ID,
        query_engine::ColumnName::PRICE
    });
    query.order_by = std::vector<query_engine::OrderByItem>{
        query_engine::OrderByItem{.expr = query_engine::ColumnName::CATEGORY_ID, .ascending = true},
        query_engine::OrderByItem{.expr = query_engine::ColumnName::PRICE, .ascending = false}
    };
    runQuery(state, query, static_cast<std::size_t>(state.range(0)));
}

void bmOrderByHiddenColumn(benchmark::State& state){
    query_engine::Query query = makeDetailQuery({query_engine::ColumnName::TRANSACTION_ID});
    query.order_by = std::vector<query_engine::OrderByItem>{
        query_engine::OrderByItem{.expr = query_engine::ColumnName::PRICE, .ascending = true}
    };
    runQuery(state, query, static_cast<std::size_t>(state.range(0)));
}

void bmOrderByWithLimit(benchmark::State& state){
    query_engine::Query query = makeDetailQuery({query_engine::ColumnName::TRANSACTION_ID});
    query.order_by = std::vector<query_engine::OrderByItem>{
        query_engine::OrderByItem{.expr = query_engine::ColumnName::PRICE, .ascending = false}
    };
    query.limit = 10;
    runQuery(state, query, 10);
}

query_engine::Query makeGroupedSumQuery(std::vector<query_engine::ColumnName> groupBy){
    return query_engine::Query{
        .projection = groupBy,
        .filter = std::nullopt,
        .aggregation = query_engine::Aggregation{
            .type = query_engine::AggregationType::SUM,
            .column = query_engine::ColumnName::PRICE
        },
        .group_by = groupBy,
        .order_by = std::nullopt,
        .limit = std::nullopt
    };
}

void bmGlobalSumQuery(benchmark::State& state){
    query_engine::Query query{
        .projection = {},
        .filter = std::nullopt,
        .aggregation = query_engine::Aggregation{
            .type = query_engine::AggregationType::SUM,
            .column = query_engine::ColumnName::PRICE
        },
        .group_by = std::nullopt,
        .order_by = std::nullopt,
        .limit = std::nullopt
    };
    runQuery(state, query, 1);
}

void bmGroupByCategory(benchmark::State& state){
    runQuery(
        state,
        makeGroupedSumQuery({query_engine::ColumnName::CATEGORY_ID}),
        10
    );
}

void bmGroupByProduct(benchmark::State& state){
    runQuery(
        state,
        makeGroupedSumQuery({query_engine::ColumnName::PRODUCT_ID}),
        100
    );
}

void bmGroupByCategoryAndProduct(benchmark::State& state){
    runQuery(
        state,
        makeGroupedSumQuery({query_engine::ColumnName::CATEGORY_ID, query_engine::ColumnName::PRODUCT_ID}),
        100
    );
}

void bmGroupByTransaction(benchmark::State& state){
    runQuery(
        state,
        makeGroupedSumQuery({query_engine::ColumnName::TRANSACTION_ID}),
        static_cast<std::size_t>(state.range(0))
    );
}

void bmComposedGroupedQuery(benchmark::State& state){
    const query_engine::Aggregation averagePrice{
        .type = query_engine::AggregationType::AVG,
        .column = query_engine::ColumnName::PRICE
    };
    query_engine::Query query{
        .projection = {query_engine::ColumnName::CATEGORY_ID},
        .filter = [](const query_engine::Row& row){ return row.transaction_id % 2 == 0; },
        .aggregation = averagePrice,
        .group_by = std::vector<query_engine::ColumnName>{query_engine::ColumnName::CATEGORY_ID},
        .order_by = std::vector<query_engine::OrderByItem>{
            query_engine::OrderByItem{.expr = averagePrice, .ascending = false}
        },
        .limit = 5
    };
    runQuery(state, query, 5);
}

void bmComposedHiddenOrderQuery(benchmark::State& state){
    query_engine::Query query = makeDetailQuery({query_engine::ColumnName::TRANSACTION_ID});
    query.filter = [](const query_engine::Row& row){ return row.category_id == 1; };
    query.order_by = std::vector<query_engine::OrderByItem>{
        query_engine::OrderByItem{.expr = query_engine::ColumnName::PRICE, .ascending = false}
    };
    query.limit = 10;
    runQuery(state, query, 10);
}

#define REGISTER_LINEAR_QUERY(functionName) \
    BENCHMARK(functionName) \
        ->Apply(benchmarkSupport::addStandardRowCounts) \
        ->Unit(benchmark::kMillisecond) \
        ->Complexity(benchmark::oN)

REGISTER_LINEAR_QUERY(bmProjectionOneColumn);
REGISTER_LINEAR_QUERY(bmProjectionThreeColumns);
REGISTER_LINEAR_QUERY(bmProjectionAllColumns);
REGISTER_LINEAR_QUERY(bmFilterZeroPercent);
REGISTER_LINEAR_QUERY(bmFilterOnePercent);
REGISTER_LINEAR_QUERY(bmFilterTenPercent);
REGISTER_LINEAR_QUERY(bmFilterFiftyPercent);
REGISTER_LINEAR_QUERY(bmFilterOneHundredPercent);
REGISTER_LINEAR_QUERY(bmLimitZero);
REGISTER_LINEAR_QUERY(bmLimitTen);
REGISTER_LINEAR_QUERY(bmGlobalSumQuery);
REGISTER_LINEAR_QUERY(bmGroupByCategory);
REGISTER_LINEAR_QUERY(bmGroupByProduct);
REGISTER_LINEAR_QUERY(bmGroupByCategoryAndProduct);
REGISTER_LINEAR_QUERY(bmComposedGroupedQuery);
REGISTER_LINEAR_QUERY(bmComposedHiddenOrderQuery);

#undef REGISTER_LINEAR_QUERY

#define REGISTER_SORT_QUERY(functionName) \
    BENCHMARK(functionName) \
        ->Apply(benchmarkSupport::addSortRowCounts) \
        ->Unit(benchmark::kMillisecond) \
        ->Complexity(benchmark::oNLogN)

REGISTER_SORT_QUERY(bmOrderByVisibleColumn);
REGISTER_SORT_QUERY(bmOrderByMultipleColumns);
REGISTER_SORT_QUERY(bmOrderByHiddenColumn);
REGISTER_SORT_QUERY(bmOrderByWithLimit);

#undef REGISTER_SORT_QUERY

BENCHMARK(bmGroupByTransaction)
    ->Apply(benchmarkSupport::addHighCardinalityRowCounts)
    ->Unit(benchmark::kMillisecond)
    ->Complexity(benchmark::oNSquared);

} // namespace
