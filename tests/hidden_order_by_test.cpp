#include "query_engine/query.hpp"
#include "query_engine/table.hpp"

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

std::uint64_t uint64Value(const ResultTable& table, std::size_t row){
    return std::get<std::uint64_t>(table.rows[row].data[0]);
}

void assertSingleOutputColumn(const ResultTable& table, const std::string& column_name){
    assert(table.column_names.size() == 1);
    assert(table.column_names[0] == column_name);
    for (const ResultRow& row : table.rows) {
        assert(row.data.size() == 1);
    }
}

} // namespace

int main(){
    const Table table = makeTestTable();

    Query hidden_columns_query{
        .projection = {ColumnName::TRANSACTION_ID},
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
    ResultTable hidden_columns_result = queryTable(table, hidden_columns_query);

    assertSingleOutputColumn(hidden_columns_result, "transaction_id");
    assert(hidden_columns_result.rows.size() == 5);
    assert(uint64Value(hidden_columns_result, 0) == 20);
    assert(uint64Value(hidden_columns_result, 1) == 50);
    assert(uint64Value(hidden_columns_result, 2) == 10);
    assert(uint64Value(hidden_columns_result, 3) == 30);
    assert(uint64Value(hidden_columns_result, 4) == 40);

    Query hidden_filtered_limit_query{
        .projection = {ColumnName::TRANSACTION_ID},
        .filter = [](Row row) {
            return row.category_id == 1;
        },
        .aggregation = std::nullopt,
        .group_by = std::nullopt,
        .order_by = std::vector<OrderByItem>{
            OrderByItem{.expr = ColumnName::PRICE, .ascending = false},
            OrderByItem{.expr = ColumnName::TRANSACTION_ID, .ascending = false}
        },
        .limit = 2
    };
    ResultTable hidden_filtered_limit_result = queryTable(table, hidden_filtered_limit_query);

    assertSingleOutputColumn(hidden_filtered_limit_result, "transaction_id");
    assert(hidden_filtered_limit_result.rows.size() == 2);
    assert(uint64Value(hidden_filtered_limit_result, 0) == 10);
    assert(uint64Value(hidden_filtered_limit_result, 1) == 50);

    Query hidden_group_key_query{
        .projection = {ColumnName::PRODUCT_ID},
        .filter = std::nullopt,
        .aggregation = std::nullopt,
        .group_by = std::vector<ColumnName>{
            ColumnName::CATEGORY_ID,
            ColumnName::PRODUCT_ID
        },
        .order_by = std::vector<OrderByItem>{
            OrderByItem{.expr = ColumnName::CATEGORY_ID, .ascending = true},
            OrderByItem{.expr = ColumnName::PRODUCT_ID, .ascending = false}
        },
        .limit = std::nullopt
    };
    ResultTable hidden_group_key_result = queryTable(table, hidden_group_key_query);

    assertSingleOutputColumn(hidden_group_key_result, "product_id");
    assert(hidden_group_key_result.rows.size() == 5);
    assert(uint64Value(hidden_group_key_result, 0) == 500);
    assert(uint64Value(hidden_group_key_result, 1) == 200);
    assert(uint64Value(hidden_group_key_result, 2) == 100);
    assert(uint64Value(hidden_group_key_result, 3) == 400);
    assert(uint64Value(hidden_group_key_result, 4) == 300);

    std::cout << "All hidden order by tests passed\n";
}
