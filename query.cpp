#include "table.hpp"
#include "query.hpp"
#include <cassert>
#include <iostream>
#include <vector>
#include <variant>

ResultTable query_table(Table table, Query query){
    std::vector<Row> filtered_rows;

    for (auto row: table.rows){
        if (!query.filter || query.filter(row)){
            filtered_rows.push_back(row);
        }
    }

    ResultTable result_table;
    for (auto row: filtered_rows){
        ResultRow result_row;
        for (ColumnName column: query.projection){
            result_row.data.push_back(getColumnValue(row, column));
        }
        result_table.rows.push_back(result_row);
    }
    return result_table;
}


ResultValue getColumnValue(Row row, ColumnName column){
    switch (column) {
        case ColumnName::TRANSACTION_ID:
            return row.transaction_id;

        case ColumnName::PRODUCT_ID:
            return row.product_id;

        case ColumnName::CATEGORY_ID:
            return row.category_id;

        case ColumnName::PRICE:
            return row.price;

        case ColumnName::QUANTITY:
            return row.quantity;

        case ColumnName::TIMESTAMP:
            return row.timestamp;
    }

    throw std::invalid_argument("Unknown column");
}


int main(){
    Table table = generate_table(20);

    Query projection_query{
        {ColumnName::PRODUCT_ID, ColumnName::CATEGORY_ID, ColumnName::PRICE},
        nullptr
    };
    ResultTable projection_result = query_table(table, projection_query);

    assert(projection_result.rows.size() == 20);
    assert(projection_result.rows[3].data.size() == 3);
    assert(std::get<std::uint64_t>(projection_result.rows[3].data[0]) == 3);
    assert(std::get<std::uint64_t>(projection_result.rows[3].data[1]) == 3);
    assert(std::get<double>(projection_result.rows[3].data[2]) == 90);

    Query filtered_query{
        {ColumnName::PRODUCT_ID, ColumnName::QUANTITY},
        [](Row row) {
            return row.category_id == 3;
        }
    };
    ResultTable filtered_result = query_table(table, filtered_query);

    assert(filtered_result.rows.size() == 2);
    assert(std::get<std::uint64_t>(filtered_result.rows[0].data[0]) == 3);
    assert(std::get<std::uint32_t>(filtered_result.rows[0].data[1]) == 51);
    assert(std::get<std::uint64_t>(filtered_result.rows[1].data[0]) == 13);
    assert(std::get<std::uint32_t>(filtered_result.rows[1].data[1]) == 27);

    Query empty_query{
        {ColumnName::TRANSACTION_ID},
        [](Row row) {
            return row.quantity > 100;
        }
    };
    ResultTable empty_result = query_table(table, empty_query);
    assert(empty_result.rows.empty());

    Query timestamp_query{
        {ColumnName::TIMESTAMP},
        nullptr
    };
    ResultTable timestamp_result = query_table(table, timestamp_query);
    assert(std::get<std::int64_t>(timestamp_result.rows[5].data[0]) == table.rows[5].timestamp);

    std::cout << "All query tests passed\n";
}
