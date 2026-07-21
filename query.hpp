#pragma once

#include <vector>
#include <functional>
#include "table.hpp"

enum ColumnName {
    TRANSACTION_ID, 
    PRODUCT_ID,     
    CATEGORY_ID,    
    PRICE,          
    QUANTITY,       
    TIMESTAMP       
};

struct Query {
    std::vector<ColumnName> projection;
    std::function<bool(Row)> filter;
};

ResultTable query_table(Table table, Query query);
ResultValue getColumnValue(Row row, ColumnName column);
