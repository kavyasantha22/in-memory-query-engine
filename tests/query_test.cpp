#include "query_engine/query.hpp"
#include "query_engine/table.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>
#include <variant>

int main(){
    Table table = generateTable(20);

    Query projection_query{
        {ColumnName::PRODUCT_ID, ColumnName::CATEGORY_ID, ColumnName::PRICE},
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt
    };
    ResultTable projection_result = queryTable(table, projection_query);

    assert(projection_result.rows.size() == 20);
    assert(projection_result.rows[3].data.size() == 3);
    assert(std::get<std::uint64_t>(projection_result.rows[3].data[0]) == 3);
    assert(std::get<std::uint64_t>(projection_result.rows[3].data[1]) == 3);
    assert(std::get<double>(projection_result.rows[3].data[2]) == 90);

    Query filtered_query{
        {ColumnName::PRODUCT_ID, ColumnName::QUANTITY},
        [](Row row) {
            return row.category_id == 3;
        },
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt
    };
    ResultTable filtered_result = queryTable(table, filtered_query);

    assert(filtered_result.rows.size() == 2);
    assert(std::get<std::uint64_t>(filtered_result.rows[0].data[0]) == 3);
    assert(std::get<std::uint32_t>(filtered_result.rows[0].data[1]) == 51);
    assert(std::get<std::uint64_t>(filtered_result.rows[1].data[0]) == 13);
    assert(std::get<std::uint32_t>(filtered_result.rows[1].data[1]) == 27);

    Query empty_query{
        {ColumnName::TRANSACTION_ID},
        [](Row row) {
            return row.quantity > 100;
        },
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt
    };
    ResultTable empty_result = queryTable(table, empty_query);
    assert(empty_result.rows.empty());

    Query timestamp_query{
        {ColumnName::TIMESTAMP},
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt
    };
    ResultTable timestamp_result = queryTable(table, timestamp_query);
    assert(std::get<std::int64_t>(timestamp_result.rows[5].data[0]) == table.rows[5].timestamp);

    std::cout << "All query tests passed\n";
}
