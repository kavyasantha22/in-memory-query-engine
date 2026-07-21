#include "table.hpp"
#include <chrono>
#include <iostream>


Table generateTable(std::uint64_t numRows){
    Table newTable;
    newTable.rows.resize(numRows);
    auto startTimestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    for (uint64_t i = 0; i < numRows; i++){
        newTable.rows[i].category_id = i;
        newTable.rows[i].product_id = i%100;
        newTable.rows[i].category_id = i%10;
        newTable.rows[i].price = (i%997) * 23 + i%13 * 7;
        newTable.rows[i].quantity = ((i%997)*17) % 97;
        newTable.rows[i].timestamp = startTimestamp + i;
    }
    return newTable;
}


int main(){
    Table table = generateTable(10);
    for (auto row: table.rows){
        std::cout
            << row.transaction_id << " "
            << row.product_id << " "
            << row.category_id << " "
            << row.price << " "
            << row.quantity << " "
            << row.timestamp << '\n';
    }
}
