#include "aggregation.hpp"
#include "query.hpp"
#include "table.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>
#include <variant>

int main(){
    Table table = generate_table(20);

    Query sum_query{
        {},
        std::nullopt,
        Aggregation{AggregationType::SUM, ColumnName::PRICE},
        std::nullopt,
        std::nullopt,
        std::nullopt
    };
    ResultTable sum_result = query_table(table, sum_query);
    assert(sum_result.rows.size() == 1);
    assert(sum_result.rows[0].data.size() == 1);
    assert(std::get<double>(sum_result.rows[0].data[0]) == 5063);

    Query filtered_count_query{
        {},
        [](Row row) {
            return row.category_id == 3;
        },
        Aggregation{AggregationType::COUNT, ColumnName::PRODUCT_ID},
        std::nullopt,
        std::nullopt,
        std::nullopt
    };
    ResultTable filtered_count_result = query_table(table, filtered_count_query);
    assert(filtered_count_result.rows.size() == 1);
    assert(filtered_count_result.rows[0].data.size() == 1);
    assert(std::get<std::uint64_t>(filtered_count_result.rows[0].data[0]) == 2);

    std::cout << "All query aggregation tests passed\n";
}
