#pragma once

#include <vector>
#include <functional>
#include <optional>
#include "table.hpp"
#include "aggregation.hpp"

enum class ColumnName {
    TRANSACTION_ID, 
    PRODUCT_ID,     
    CATEGORY_ID,    
    PRICE,          
    QUANTITY,       
    TIMESTAMP       
};

struct Query {
    std::vector<ColumnName> projection;
    std::optional<std::function<bool(Row)>> filter;
    std::optional<Aggregation> aggregation;
};

ResultTable query_table(Table table, Query query);
ResultValue getColumnValue(Row row, ColumnName column);
