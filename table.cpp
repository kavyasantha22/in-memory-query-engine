#include "table.hpp"
#include <chrono>


Table generate_table(std::uint64_t numRows){
    Table new_table;
    new_table.column_names = {
        ColumnName::TRANSACTION_ID,
        ColumnName::PRODUCT_ID,
        ColumnName::CATEGORY_ID,
        ColumnName::PRICE,
        ColumnName::QUANTITY,
        ColumnName::TIMESTAMP
    };
    new_table.rows.resize(numRows);
    auto startTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    for (uint64_t i = 0; i < numRows; i++){
        new_table.rows[i].transaction_id = i;
        new_table.rows[i].product_id = i%100;
        new_table.rows[i].category_id = i%10;
        new_table.rows[i].price = (i%997) * 23 + i%13 * 7;
        new_table.rows[i].quantity = ((i%997)*17) % 97;
        new_table.rows[i].timestamp = startTimestamp + i;
    }
    return new_table;
}
