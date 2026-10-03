#include "query_engine/query.hpp"
#include "query_engine/table.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>
#include <variant>

int main(){
    query_engine::Table table = query_engine::generateTable(20);

    query_engine::Query projection_query{
        {query_engine::ColumnName::PRODUCT_ID, query_engine::ColumnName::CATEGORY_ID, query_engine::ColumnName::PRICE},
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt
    };
    query_engine::ResultTable projection_result = query_engine::queryTable(table, projection_query);

    assert(projection_result.rows.size() == 20);
    assert(projection_result.rows[3].data.size() == 3);
    assert(std::get<std::uint64_t>(projection_result.rows[3].data[0]) == 3);
    assert(std::get<std::uint64_t>(projection_result.rows[3].data[1]) == 3);
    assert(std::get<double>(projection_result.rows[3].data[2]) == 90);

    query_engine::Query filtered_query{
        {query_engine::ColumnName::PRODUCT_ID, query_engine::ColumnName::QUANTITY},
        [](query_engine::Row row) {
            return row.category_id == 3;
        },
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt
    };
    query_engine::ResultTable filtered_result = query_engine::queryTable(table, filtered_query);

    assert(filtered_result.rows.size() == 2);
    assert(std::get<std::uint64_t>(filtered_result.rows[0].data[0]) == 3);
    assert(std::get<std::uint32_t>(filtered_result.rows[0].data[1]) == 51);
    assert(std::get<std::uint64_t>(filtered_result.rows[1].data[0]) == 13);
    assert(std::get<std::uint32_t>(filtered_result.rows[1].data[1]) == 27);

    query_engine::Query empty_query{
        {query_engine::ColumnName::TRANSACTION_ID},
        [](query_engine::Row row) {
            return row.quantity > 100;
        },
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt
    };
    query_engine::ResultTable empty_result = query_engine::queryTable(table, empty_query);
    assert(empty_result.rows.empty());

    query_engine::Query timestamp_query{
        {query_engine::ColumnName::TIMESTAMP},
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt
    };
    query_engine::ResultTable timestamp_result = query_engine::queryTable(table, timestamp_query);
    assert(std::get<std::int64_t>(timestamp_result.rows[5].data[0]) == table.rows[5].timestamp);

    std::cout << "All query tests passed\n";
}
