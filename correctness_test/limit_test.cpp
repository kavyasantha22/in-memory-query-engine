#include "aggregation.hpp"
#include "query.hpp"
#include "table.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>
#include <variant>
#include <vector>

namespace {

std::uint64_t uint64Value(const ResultTable& table, std::size_t row, std::size_t column){
    return std::get<std::uint64_t>(table.rows[row].data[column]);
}

double doubleValue(const ResultTable& table, std::size_t row, std::size_t column){
    return std::get<double>(table.rows[row].data[column]);
}

} // namespace

int main(){
    const Table table = generate_table(10);

    Query basic_limit_query{
        .projection = {ColumnName::TRANSACTION_ID},
        .filter = std::nullopt,
        .aggregation = std::nullopt,
        .group_by = std::nullopt,
        .order_by = std::nullopt,
        .limit = 3
    };
    ResultTable basic_limit_result = query_table(table, basic_limit_query);

    assert(basic_limit_result.rows.size() == 3);
    assert(uint64Value(basic_limit_result, 0, 0) == 0);
    assert(uint64Value(basic_limit_result, 1, 0) == 1);
    assert(uint64Value(basic_limit_result, 2, 0) == 2);

    Query zero_limit_query{
        .projection = {ColumnName::TRANSACTION_ID},
        .filter = std::nullopt,
        .aggregation = std::nullopt,
        .group_by = std::nullopt,
        .order_by = std::nullopt,
        .limit = 0
    };
    ResultTable zero_limit_result = query_table(table, zero_limit_query);

    assert(zero_limit_result.column_names.size() == 1);
    assert(zero_limit_result.rows.empty());

    Query equal_limit_query{
        .projection = {ColumnName::TRANSACTION_ID},
        .filter = std::nullopt,
        .aggregation = std::nullopt,
        .group_by = std::nullopt,
        .order_by = std::nullopt,
        .limit = 10
    };
    ResultTable equal_limit_result = query_table(table, equal_limit_query);
    assert(equal_limit_result.rows.size() == 10);

    Query oversized_limit_query{
        .projection = {ColumnName::TRANSACTION_ID},
        .filter = std::nullopt,
        .aggregation = std::nullopt,
        .group_by = std::nullopt,
        .order_by = std::nullopt,
        .limit = 20
    };
    ResultTable oversized_limit_result = query_table(table, oversized_limit_query);
    assert(oversized_limit_result.rows.size() == 10);

    Query filtered_limit_query{
        .projection = {ColumnName::TRANSACTION_ID},
        .filter = [](Row row) {
            return row.transaction_id >= 5;
        },
        .aggregation = std::nullopt,
        .group_by = std::nullopt,
        .order_by = std::nullopt,
        .limit = 2
    };
    ResultTable filtered_limit_result = query_table(table, filtered_limit_query);

    assert(filtered_limit_result.rows.size() == 2);
    assert(uint64Value(filtered_limit_result, 0, 0) == 5);
    assert(uint64Value(filtered_limit_result, 1, 0) == 6);

    Query ordered_limit_query{
        .projection = {ColumnName::TRANSACTION_ID},
        .filter = std::nullopt,
        .aggregation = std::nullopt,
        .group_by = std::nullopt,
        .order_by = std::vector<OrderByItem>{
            OrderByItem{
                .expr = ColumnName::TRANSACTION_ID,
                .ascending = false
            }
        },
        .limit = 3
    };
    ResultTable ordered_limit_result = query_table(table, ordered_limit_query);

    assert(ordered_limit_result.rows.size() == 3);
    assert(uint64Value(ordered_limit_result, 0, 0) == 9);
    assert(uint64Value(ordered_limit_result, 1, 0) == 8);
    assert(uint64Value(ordered_limit_result, 2, 0) == 7);

    Query grouped_limit_query{
        .projection = {ColumnName::CATEGORY_ID},
        .filter = std::nullopt,
        .aggregation = Aggregation{AggregationType::SUM, ColumnName::PRICE},
        .group_by = std::vector<ColumnName>{ColumnName::CATEGORY_ID},
        .order_by = std::vector<OrderByItem>{
            OrderByItem{
                .expr = Aggregation{AggregationType::SUM, ColumnName::PRICE},
                .ascending = false
            }
        },
        .limit = 3
    };
    ResultTable grouped_limit_result = query_table(generate_table(20), grouped_limit_query);

    assert(grouped_limit_result.rows.size() == 3);
    assert(doubleValue(grouped_limit_result, 0, 1) >= doubleValue(grouped_limit_result, 1, 1));
    assert(doubleValue(grouped_limit_result, 1, 1) >= doubleValue(grouped_limit_result, 2, 1));

    Query no_limit_query{
        .projection = {ColumnName::TRANSACTION_ID},
        .filter = std::nullopt,
        .aggregation = std::nullopt,
        .group_by = std::nullopt,
        .order_by = std::nullopt,
        .limit = std::nullopt
    };
    ResultTable no_limit_result = query_table(table, no_limit_query);

    assert(no_limit_result.rows.size() == 10);
    assert(uint64Value(no_limit_result, 0, 0) == 0);
    assert(uint64Value(no_limit_result, 9, 0) == 9);

    std::cout << "All limit tests passed\n";
}
