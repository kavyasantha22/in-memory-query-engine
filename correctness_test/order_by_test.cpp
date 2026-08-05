#include "query.hpp"
#include "table.hpp"
#include "formatter.hpp"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>
#include <variant>
#include <vector>

namespace {

Table makeTestTable(){
    return Table{
        .column_names = {
            ColumnName::TRANSACTION_ID,
            ColumnName::PRODUCT_ID,
            ColumnName::CATEGORY_ID,
            ColumnName::PRICE,
            ColumnName::QUANTITY,
            ColumnName::TIMESTAMP
        },
        .rows = {
            Row{40, 400, 2, 30.0, 4, 104},
            Row{10, 100, 1, 20.0, 1, 101},
            Row{30, 300, 2, 15.0, 3, 103},
            Row{20, 200, 1, 10.0, 2, 102},
            Row{50, 500, 1, 10.0, 5, 105}
        }
    };
}

std::uint64_t uint64Value(const ResultTable& table, std::size_t row, std::size_t column){
    return std::get<std::uint64_t>(table.rows[row].data[column]);
}

double doubleValue(const ResultTable& table, std::size_t row, std::size_t column){
    return std::get<double>(table.rows[row].data[column]);
}

} // namespace

int main(){
    const Table table = makeTestTable();
    // printSqlTable("initial", table);

    Query single_column_query{
        .projection = {ColumnName::TRANSACTION_ID},
        .filter = std::nullopt,
        .aggregation = std::nullopt,
        .group_by = std::nullopt,
        .order_by = std::vector<OrderByItem>{
            OrderByItem{
                .expr = ColumnName::TRANSACTION_ID,
                .ascending = true
            }
        },
        .limit = std::nullopt
    };
    ResultTable single_column_result = queryTable(table, single_column_query);
    // printSqlTable("single_column", single_column_result);

    assert(single_column_result.rows.size() == 5);
    assert(uint64Value(single_column_result, 0, 0) == 10);
    assert(uint64Value(single_column_result, 1, 0) == 20);
    assert(uint64Value(single_column_result, 2, 0) == 30);
    assert(uint64Value(single_column_result, 3, 0) == 40);
    assert(uint64Value(single_column_result, 4, 0) == 50);

    Query multiple_column_query{
        .projection = {
            ColumnName::CATEGORY_ID,
            ColumnName::PRICE,
            ColumnName::TRANSACTION_ID
        },
        .filter = std::nullopt,
        .aggregation = std::nullopt,
        .group_by = std::nullopt,
        .order_by = std::vector<OrderByItem>{
            OrderByItem{.expr = ColumnName::CATEGORY_ID, .ascending = true},
            OrderByItem{.expr = ColumnName::PRICE, .ascending = true},
            OrderByItem{.expr = ColumnName::TRANSACTION_ID, .ascending = true}
        },
        .limit = std::nullopt
    };
    ResultTable multiple_column_result = queryTable(table, multiple_column_query);

    assert(uint64Value(multiple_column_result, 0, 2) == 20);
    assert(uint64Value(multiple_column_result, 1, 2) == 50);
    assert(uint64Value(multiple_column_result, 2, 2) == 10);
    assert(uint64Value(multiple_column_result, 3, 2) == 30);
    assert(uint64Value(multiple_column_result, 4, 2) == 40);

    Query filtered_query{
        .projection = {ColumnName::TRANSACTION_ID, ColumnName::PRICE},
        .filter = [](Row row) {
            return row.category_id == 1;
        },
        .aggregation = std::nullopt,
        .group_by = std::nullopt,
        .order_by = std::vector<OrderByItem>{
            OrderByItem{.expr = ColumnName::PRICE, .ascending = true},
            OrderByItem{.expr = ColumnName::TRANSACTION_ID, .ascending = true}
        },
        .limit = std::nullopt
    };
    ResultTable filtered_result = queryTable(table, filtered_query);

    assert(filtered_result.rows.size() == 3);
    assert(uint64Value(filtered_result, 0, 0) == 20);
    assert(doubleValue(filtered_result, 0, 1) == 10.0);
    assert(uint64Value(filtered_result, 1, 0) == 50);
    assert(doubleValue(filtered_result, 1, 1) == 10.0);
    assert(uint64Value(filtered_result, 2, 0) == 10);
    assert(doubleValue(filtered_result, 2, 1) == 20.0);

    Query descending_query{
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
        .limit = std::nullopt
    };
    ResultTable descending_result = queryTable(table, descending_query);

    assert(uint64Value(descending_result, 0, 0) == 50);
    assert(uint64Value(descending_result, 1, 0) == 40);
    assert(uint64Value(descending_result, 2, 0) == 30);
    assert(uint64Value(descending_result, 3, 0) == 20);
    assert(uint64Value(descending_result, 4, 0) == 10);

    Query grouped_query{
        .projection = {ColumnName::CATEGORY_ID},
        .filter = std::nullopt,
        .aggregation = std::nullopt,
        .group_by = std::vector<ColumnName>{ColumnName::CATEGORY_ID},
        .order_by = std::vector<OrderByItem>{
            OrderByItem{.expr = ColumnName::CATEGORY_ID, .ascending = true}
        },
        .limit = std::nullopt
    };
    ResultTable grouped_result = queryTable(table, grouped_query);

    assert(grouped_result.rows.size() == 2);
    assert(uint64Value(grouped_result, 0, 0) == 1);
    assert(uint64Value(grouped_result, 1, 0) == 2);

    Query aggregate_query{
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
        .limit = std::nullopt
    };
    ResultTable aggregate_result = queryTable(table, aggregate_query);

    assert(aggregate_result.rows.size() == 2);
    assert(uint64Value(aggregate_result, 0, 0) == 2);
    assert(doubleValue(aggregate_result, 0, 1) == 45.0);
    assert(uint64Value(aggregate_result, 1, 0) == 1);
    assert(doubleValue(aggregate_result, 1, 1) == 40.0);

    Query no_order_query{
        .projection = {ColumnName::TRANSACTION_ID},
        .filter = std::nullopt,
        .aggregation = std::nullopt,
        .group_by = std::nullopt,
        .order_by = std::nullopt,
        .limit = std::nullopt
    };
    ResultTable no_order_result = queryTable(table, no_order_query);

    assert(uint64Value(no_order_result, 0, 0) == 40);
    assert(uint64Value(no_order_result, 1, 0) == 10);
    assert(uint64Value(no_order_result, 2, 0) == 30);
    assert(uint64Value(no_order_result, 3, 0) == 20);
    assert(uint64Value(no_order_result, 4, 0) == 50);

    std::cout << "All order by tests passed\n";
}
