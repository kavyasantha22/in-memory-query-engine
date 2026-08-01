#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <variant>

using ResultValue = std::variant<std::uint64_t, std::uint32_t, double, std::int64_t>;

enum class ColumnName {
    TRANSACTION_ID, 
    PRODUCT_ID,     
    CATEGORY_ID,    
    PRICE,          
    QUANTITY,       
    TIMESTAMP,
};

struct Row {
    std::uint64_t transaction_id;
    std::uint64_t product_id;
    std::uint64_t category_id;
    double price;
    std::uint32_t quantity;
    std::int64_t timestamp;
};

struct Table {
    std::vector<ColumnName> column_names;
    std::vector<Row> rows;
};


struct ResultRow {
    std::vector<ResultValue> data;
};


struct ResultTable {
    std::vector<std::string> column_names;
    std::vector<ResultRow> rows;
};

struct Group {
    std::vector<ColumnName> key_columns;
    std::vector<ResultValue> key;
    std::vector<Row> rows;
};

Table generate_table(std::uint64_t numRows);

std::string columnNameToString(ColumnName col);

ResultValue getColumnValue(Row row, ColumnName column);
