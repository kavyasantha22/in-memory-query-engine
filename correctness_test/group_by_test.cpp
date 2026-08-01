#include "aggregation.hpp"
#include "formatter.hpp"
#include "query.hpp"
#include "table.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

int main(){
    Table table = generate_table(20);
    printSqlTable("generated table", table);

    Query q = {
        .projection = {
            ColumnName::TRANSACTION_ID,
            ColumnName::PRODUCT_ID,
            ColumnName::CATEGORY_ID,
            ColumnName::PRICE,
            ColumnName::QUANTITY,
            ColumnName::TIMESTAMP
        },
        .filter = std::nullopt,
        .aggregation = Aggregation{AggregationType::SUM, ColumnName::PRICE},
        .group_by = std::vector<ColumnName>{
            ColumnName::CATEGORY_ID
        },
        .order_by = std::nullopt,
        .limit = std::nullopt
    };

    ResultTable result = query_table(table, q);
    printSqlTable("group by result", result);
}
