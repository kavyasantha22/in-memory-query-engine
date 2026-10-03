#include "query_engine/aggregation.hpp"
#include "query_engine/query.hpp"
#include "query_engine/table.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>
#include <variant>

int main(){
    query_engine::Table table = query_engine::generateTable(20);

    query_engine::Query sum_query{
        {},
        std::nullopt,
        query_engine::Aggregation{query_engine::AggregationType::SUM, query_engine::ColumnName::PRICE},
        std::nullopt,
        std::nullopt,
        std::nullopt
    };
    query_engine::ResultTable sum_result = query_engine::queryTable(table, sum_query);
    assert(sum_result.rows.size() == 1);
    assert(sum_result.rows[0].data.size() == 1);
    assert(std::get<double>(sum_result.rows[0].data[0]) == 5063);

    query_engine::Query filtered_count_query{
        {},
        [](query_engine::Row row) {
            return row.category_id == 3;
        },
        query_engine::Aggregation{query_engine::AggregationType::COUNT, query_engine::ColumnName::PRODUCT_ID},
        std::nullopt,
        std::nullopt,
        std::nullopt
    };
    query_engine::ResultTable filtered_count_result = query_engine::queryTable(table, filtered_count_query);
    assert(filtered_count_result.rows.size() == 1);
    assert(filtered_count_result.rows[0].data.size() == 1);
    assert(std::get<std::uint64_t>(filtered_count_result.rows[0].data[0]) == 2);

    std::cout << "All query aggregation tests passed\n";
}
