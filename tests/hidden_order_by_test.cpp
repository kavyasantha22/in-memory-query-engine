#include "query_engine/query.hpp"
#include "query_engine/table.hpp"

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

std::uint64_t uint64Value(const query_engine::ResultTable& table, std::size_t row){
    return std::get<std::uint64_t>(table.rows[row].data[0]);
}

void assertSingleOutputColumn(const query_engine::ResultTable& table, const std::string& column_name){
    assert(table.column_names.size() == 1);
    assert(table.column_names[0] == column_name);
    for (const query_engine::ResultRow& row : table.rows) {
        assert(row.data.size() == 1);
    }
}

} // namespace

int main(){
    const query_engine::Table table = makeTestTable();

    query_engine::Query hidden_columns_query{
        .projection = {query_engine::ColumnName::TRANSACTION_ID},
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
    query_engine::ResultTable hidden_columns_result = query_engine::queryTable(table, hidden_columns_query);

    assertSingleOutputColumn(hidden_columns_result, "transaction_id");
    assert(hidden_columns_result.rows.size() == 5);
    assert(uint64Value(hidden_columns_result, 0) == 20);
    assert(uint64Value(hidden_columns_result, 1) == 50);
    assert(uint64Value(hidden_columns_result, 2) == 10);
    assert(uint64Value(hidden_columns_result, 3) == 30);
    assert(uint64Value(hidden_columns_result, 4) == 40);

    query_engine::Query hidden_filtered_limit_query{
        .projection = {query_engine::ColumnName::TRANSACTION_ID},
        .filter = [](query_engine::Row row) {
            return row.category_id == 1;
        },
        .aggregation = std::nullopt,
        .group_by = std::nullopt,
        .order_by = std::vector<query_engine::OrderByItem>{
            query_engine::OrderByItem{.expr = query_engine::ColumnName::PRICE, .ascending = false},
            query_engine::OrderByItem{.expr = query_engine::ColumnName::TRANSACTION_ID, .ascending = false}
        },
        .limit = 2
    };
    query_engine::ResultTable hidden_filtered_limit_result = query_engine::queryTable(table, hidden_filtered_limit_query);

    assertSingleOutputColumn(hidden_filtered_limit_result, "transaction_id");
    assert(hidden_filtered_limit_result.rows.size() == 2);
    assert(uint64Value(hidden_filtered_limit_result, 0) == 10);
    assert(uint64Value(hidden_filtered_limit_result, 1) == 50);

    query_engine::Query hidden_group_key_query{
        .projection = {query_engine::ColumnName::PRODUCT_ID},
        .filter = std::nullopt,
        .aggregation = std::nullopt,
        .group_by = std::vector<query_engine::ColumnName>{
            query_engine::ColumnName::CATEGORY_ID,
            query_engine::ColumnName::PRODUCT_ID
        },
        .order_by = std::vector<query_engine::OrderByItem>{
            query_engine::OrderByItem{.expr = query_engine::ColumnName::CATEGORY_ID, .ascending = true},
            query_engine::OrderByItem{.expr = query_engine::ColumnName::PRODUCT_ID, .ascending = false}
        },
        .limit = std::nullopt
    };
    query_engine::ResultTable hidden_group_key_result = query_engine::queryTable(table, hidden_group_key_query);

    assertSingleOutputColumn(hidden_group_key_result, "product_id");
    assert(hidden_group_key_result.rows.size() == 5);
    assert(uint64Value(hidden_group_key_result, 0) == 500);
    assert(uint64Value(hidden_group_key_result, 1) == 200);
    assert(uint64Value(hidden_group_key_result, 2) == 100);
    assert(uint64Value(hidden_group_key_result, 3) == 400);
    assert(uint64Value(hidden_group_key_result, 4) == 300);

    std::cout << "All hidden order by tests passed\n";
}
