#include "query_engine/query.hpp"
#include "query_engine/table.hpp"
#include "query_engine/formatter.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>
#include <variant>
#include <vector>

namespace {

query_engine::Table makeTestTable(){
    return query_engine::Table{
        .column_names = {
            query_engine::ColumnName::TRANSACTION_ID,
            query_engine::ColumnName::PRODUCT_ID,
            query_engine::ColumnName::CATEGORY_ID,
            query_engine::ColumnName::PRICE,
            query_engine::ColumnName::QUANTITY,
            query_engine::ColumnName::TIMESTAMP
        },
        .rows = {
            query_engine::Row{40, 400, 2, 30.0, 4, 104},
            query_engine::Row{10, 100, 1, 20.0, 1, 101},
            query_engine::Row{30, 300, 2, 15.0, 3, 103},
            query_engine::Row{20, 200, 1, 10.0, 2, 102},
            query_engine::Row{50, 500, 1, 10.0, 5, 105}
        }
    };
}

std::uint64_t uint64Value(const query_engine::ResultTable& table, std::size_t row, std::size_t column){
    return std::get<std::uint64_t>(table.rows[row].data[column]);
}

double doubleValue(const query_engine::ResultTable& table, std::size_t row, std::size_t column){
    return std::get<double>(table.rows[row].data[column]);
}

} // namespace

int main(){
    const query_engine::Table table = makeTestTable();
    // query_engine::printSqlTable("initial", table);

    query_engine::Query single_column_query{
        .projection = {query_engine::ColumnName::TRANSACTION_ID},
        .filter = std::nullopt,
        .aggregation = std::nullopt,
        .group_by = std::nullopt,
        .order_by = std::vector<query_engine::OrderByItem>{
            query_engine::OrderByItem{
                .expr = query_engine::ColumnName::TRANSACTION_ID,
                .ascending = true
            }
        },
        .limit = std::nullopt
    };
    query_engine::ResultTable single_column_result = query_engine::queryTable(table, single_column_query);
    // query_engine::printSqlTable("single_column", single_column_result);

    assert(single_column_result.rows.size() == 5);
    assert(uint64Value(single_column_result, 0, 0) == 10);
    assert(uint64Value(single_column_result, 1, 0) == 20);
    assert(uint64Value(single_column_result, 2, 0) == 30);
    assert(uint64Value(single_column_result, 3, 0) == 40);
    assert(uint64Value(single_column_result, 4, 0) == 50);

    query_engine::Query multiple_column_query{
        .projection = {
            query_engine::ColumnName::CATEGORY_ID,
            query_engine::ColumnName::PRICE,
            query_engine::ColumnName::TRANSACTION_ID
        },
        .filter = std::nullopt,
        .aggregation = std::nullopt,
        .group_by = std::nullopt,
        .order_by = std::vector<query_engine::OrderByItem>{
            query_engine::OrderByItem{.expr = query_engine::ColumnName::CATEGORY_ID, .ascending = true},
            query_engine::OrderByItem{.expr = query_engine::ColumnName::PRICE, .ascending = true},
            query_engine::OrderByItem{.expr = query_engine::ColumnName::TRANSACTION_ID, .ascending = true}
        },
        .limit = std::nullopt
    };
    query_engine::ResultTable multiple_column_result = query_engine::queryTable(table, multiple_column_query);

    assert(uint64Value(multiple_column_result, 0, 2) == 20);
    assert(uint64Value(multiple_column_result, 1, 2) == 50);
    assert(uint64Value(multiple_column_result, 2, 2) == 10);
    assert(uint64Value(multiple_column_result, 3, 2) == 30);
    assert(uint64Value(multiple_column_result, 4, 2) == 40);

    query_engine::Query filtered_query{
        .projection = {query_engine::ColumnName::TRANSACTION_ID, query_engine::ColumnName::PRICE},
        .filter = [](query_engine::Row row) {
            return row.category_id == 1;
        },
        .aggregation = std::nullopt,
        .group_by = std::nullopt,
        .order_by = std::vector<query_engine::OrderByItem>{
            query_engine::OrderByItem{.expr = query_engine::ColumnName::PRICE, .ascending = true},
            query_engine::OrderByItem{.expr = query_engine::ColumnName::TRANSACTION_ID, .ascending = true}
        },
        .limit = std::nullopt
    };
    query_engine::ResultTable filtered_result = query_engine::queryTable(table, filtered_query);

    assert(filtered_result.rows.size() == 3);
    assert(uint64Value(filtered_result, 0, 0) == 20);
    assert(doubleValue(filtered_result, 0, 1) == 10.0);
    assert(uint64Value(filtered_result, 1, 0) == 50);
    assert(doubleValue(filtered_result, 1, 1) == 10.0);
    assert(uint64Value(filtered_result, 2, 0) == 10);
    assert(doubleValue(filtered_result, 2, 1) == 20.0);

    query_engine::Query descending_query{
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
        .limit = std::nullopt
    };
    query_engine::ResultTable descending_result = query_engine::queryTable(table, descending_query);

    assert(uint64Value(descending_result, 0, 0) == 50);
    assert(uint64Value(descending_result, 1, 0) == 40);
    assert(uint64Value(descending_result, 2, 0) == 30);
    assert(uint64Value(descending_result, 3, 0) == 20);
    assert(uint64Value(descending_result, 4, 0) == 10);

    query_engine::Query grouped_query{
        .projection = {query_engine::ColumnName::CATEGORY_ID},
        .filter = std::nullopt,
        .aggregation = std::nullopt,
        .group_by = std::vector<query_engine::ColumnName>{query_engine::ColumnName::CATEGORY_ID},
        .order_by = std::vector<query_engine::OrderByItem>{
            query_engine::OrderByItem{.expr = query_engine::ColumnName::CATEGORY_ID, .ascending = true}
        },
        .limit = std::nullopt
    };
    query_engine::ResultTable grouped_result = query_engine::queryTable(table, grouped_query);

    assert(grouped_result.rows.size() == 2);
    assert(uint64Value(grouped_result, 0, 0) == 1);
    assert(uint64Value(grouped_result, 1, 0) == 2);

    query_engine::Query aggregate_query{
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
        .limit = std::nullopt
    };
    query_engine::ResultTable aggregate_result = query_engine::queryTable(table, aggregate_query);

    assert(aggregate_result.rows.size() == 2);
    assert(uint64Value(aggregate_result, 0, 0) == 2);
    assert(doubleValue(aggregate_result, 0, 1) == 45.0);
    assert(uint64Value(aggregate_result, 1, 0) == 1);
    assert(doubleValue(aggregate_result, 1, 1) == 40.0);

    query_engine::Query no_order_query{
        .projection = {query_engine::ColumnName::TRANSACTION_ID},
        .filter = std::nullopt,
        .aggregation = std::nullopt,
        .group_by = std::nullopt,
        .order_by = std::nullopt,
        .limit = std::nullopt
    };
    query_engine::ResultTable no_order_result = query_engine::queryTable(table, no_order_query);

    assert(uint64Value(no_order_result, 0, 0) == 40);
    assert(uint64Value(no_order_result, 1, 0) == 10);
    assert(uint64Value(no_order_result, 2, 0) == 30);
    assert(uint64Value(no_order_result, 3, 0) == 20);
    assert(uint64Value(no_order_result, 4, 0) == 50);

    std::cout << "All order by tests passed\n";
}
