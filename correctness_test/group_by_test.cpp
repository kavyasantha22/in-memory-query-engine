#include "aggregation.hpp"
#include "query.hpp"
#include "table.hpp"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <variant>
#include <vector>

int main(){
    Table table = generate_table(20);
    for (auto r : table.rows){
        std::cout
            << "transaction_id=" << r.transaction_id << ", "
            << "product_id=" << r.product_id << ", "
            << "category_id=" << r.category_id << ", "
            << "price=" << r.price << ", "
            << "quantity=" << r.quantity << ", "
            << "timestamp=" << r.timestamp << "\n";
    }
    Query q = {
        .projection = {
            ColumnName::TRANSACTION_ID,
            ColumnName::PRODUCT_ID,
            ColumnName::CATEGORY_ID,
            ColumnName::PRICE,
            ColumnName::QUANTITY,
            ColumnName::TIMESTAMP
        },
        .aggregation = Aggregation{AggregationType::SUM, ColumnName::PRICE},
        .group_by = std::vector<ColumnName>{
            ColumnName::CATEGORY_ID
        }
    };
    std::cout<<std::endl;
    std::cout<<std::endl;
    ResultTable result = query_table(table, q);
    std::cout<<result.rows[0].data.size()<<std::endl;
    for (auto row: result.rows){
        for (auto v: row.data){
            std::visit([](auto value) {
                std::cout << value << " ";
            }, v);
        }
        std::cout<<std::endl;
    }
}
