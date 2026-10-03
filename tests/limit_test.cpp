#include "query_engine/aggregation.hpp"
#include "query_engine/query.hpp"
#include "query_engine/table.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>
#include <variant>
#include <vector>

namespace {

std::uint64_t uint64Value(const query_engine::ResultTable& table, std::size_t row, std::size_t column){
    return std::get<std::uint64_t>(table.rows[row].data[column]);
}

double doubleValue(const query_engine::ResultTable& table, std::size_t row, std::size_t column){
    return std::get<double>(table.rows[row].data[column]);
}

} // namespace

int main(){
    const query_engine::Table table = query_engine::generateTable(10);

    query_engine::Query basic_limit_query{
        .projection = {query_engine::ColumnName::TRANSACTION_ID},
        .filter = std::nullopt,
        .aggregation = std::nullopt,
        .group_by = std::nullopt,
        .order_by = std::nullopt,
        .limit = 3
    };
    query_engine::ResultTable basic_limit_result = query_engine::queryTable(table, basic_limit_query);

    assert(basic_limit_result.rows.size() == 3);
    assert(uint64Value(basic_limit_result, 0, 0) == 0);
    assert(uint64Value(basic_limit_result, 1, 0) == 1);
    assert(uint64Value(basic_limit_result, 2, 0) == 2);

    query_engine::Query zero_limit_query{
        .projection = {query_engine::ColumnName::TRANSACTION_ID},
        .filter = std::nullopt,
        .aggregation = std::nullopt,
        .group_by = std::nullopt,
        .order_by = std::nullopt,
        .limit = 0
    };
    query_engine::ResultTable zero_limit_result = query_engine::queryTable(table, zero_limit_query);

    assert(zero_limit_result.column_names.size() == 1);
    assert(zero_limit_result.rows.empty());

    query_engine::Query equal_limit_query{
        .projection = {query_engine::ColumnName::TRANSACTION_ID},
        .filter = std::nullopt,
        .aggregation = std::nullopt,
        .group_by = std::nullopt,
        .order_by = std::nullopt,
        .limit = 10
    };
    query_engine::ResultTable equal_limit_result = query_engine::queryTable(table, equal_limit_query);
    assert(equal_limit_result.rows.size() == 10);

    query_engine::Query oversized_limit_query{
        .projection = {query_engine::ColumnName::TRANSACTION_ID},
        .filter = std::nullopt,
        .aggregation = std::nullopt,
        .group_by = std::nullopt,
        .order_by = std::nullopt,
        .limit = 20
    };
    query_engine::ResultTable oversized_limit_result = query_engine::queryTable(table, oversized_limit_query);
    assert(oversized_limit_result.rows.size() == 10);

    query_engine::Query filtered_limit_query{
        .projection = {query_engine::ColumnName::TRANSACTION_ID},
        .filter = [](query_engine::Row row) {
            return row.transaction_id >= 5;
        },
        .aggregation = std::nullopt,
        .group_by = std::nullopt,
        .order_by = std::nullopt,
        .limit = 2
    };
    query_engine::ResultTable filtered_limit_result = query_engine::queryTable(table, filtered_limit_query);

    assert(filtered_limit_result.rows.size() == 2);
    assert(uint64Value(filtered_limit_result, 0, 0) == 5);
    assert(uint64Value(filtered_limit_result, 1, 0) == 6);

    query_engine::Query ordered_limit_query{
        .projection = {query_engine::ColumnName::TRANSACTION_ID},
        .filter = std::nullopt,
        .aggregation = std::nullopt,
        .group_by = std::nullopt,
        .order_by = std::vector<query_engine::OrderByItem>{
            query_engine::OrderByItem{
                .expr = query_engine::ColumnName::TRANSACTION_ID,
                .ascending = false
            }
        },
        .limit = 3
    };
    query_engine::ResultTable ordered_limit_result = query_engine::queryTable(table, ordered_limit_query);

    assert(ordered_limit_result.rows.size() == 3);
    assert(uint64Value(ordered_limit_result, 0, 0) == 9);
    assert(uint64Value(ordered_limit_result, 1, 0) == 8);
    assert(uint64Value(ordered_limit_result, 2, 0) == 7);

    query_engine::Query grouped_limit_query{
        .projection = {query_engine::ColumnName::CATEGORY_ID},
        .filter = std::nullopt,
        .aggregation = query_engine::Aggregation{query_engine::AggregationType::SUM, query_engine::ColumnName::PRICE},
        .group_by = std::vector<query_engine::ColumnName>{query_engine::ColumnName::CATEGORY_ID},
        .order_by = std::vector<query_engine::OrderByItem>{
            query_engine::OrderByItem{
                .expr = query_engine::Aggregation{query_engine::AggregationType::SUM, query_engine::ColumnName::PRICE},
                .ascending = false
            }
        },
        .limit = 3
    };
    query_engine::ResultTable grouped_limit_result = query_engine::queryTable(query_engine::generateTable(20), grouped_limit_query);

    assert(grouped_limit_result.rows.size() == 3);
    assert(doubleValue(grouped_limit_result, 0, 1) >= doubleValue(grouped_limit_result, 1, 1));
    assert(doubleValue(grouped_limit_result, 1, 1) >= doubleValue(grouped_limit_result, 2, 1));

    query_engine::Query no_limit_query{
        .projection = {query_engine::ColumnName::TRANSACTION_ID},
        .filter = std::nullopt,
        .aggregation = std::nullopt,
        .group_by = std::nullopt,
        .order_by = std::nullopt,
        .limit = std::nullopt
    };
    query_engine::ResultTable no_limit_result = query_engine::queryTable(table, no_limit_query);

    assert(no_limit_result.rows.size() == 10);
    assert(uint64Value(no_limit_result, 0, 0) == 0);
    assert(uint64Value(no_limit_result, 9, 0) == 9);

    std::cout << "All limit tests passed\n";
}
